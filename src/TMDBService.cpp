#include "TMDBService.h"
#include <iomanip>
#include <sstream>

const std::string TMDBService::API_BASE = "https://api.themoviedb.org/3";
const std::string TMDBService::IMAGE_BASE = "https://image.tmdb.org/t/p/";

namespace{
	// Percent-encodes a query string component so film titles with spaces,
	// punctuation, or non-ASCII characters survive the trip to the API.
	std::string urlEncode(const std::string & value){
		std::ostringstream escaped;
		escaped.fill('0');
		escaped << std::hex;
		for(unsigned char c : value){
			if(isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~'){
				escaped << c;
			}else{
				escaped << '%' << std::uppercase << std::setw(2) << int(c) << std::nouppercase << std::setw(0);
			}
		}
		return escaped.str();
	}
}

TMDBService::TMDBService(){
	ofRegisterURLNotification(this);
}

TMDBService::~TMDBService(){
	ofUnregisterURLNotification(this);
}

void TMDBService::setup(const std::string & apiKeyIn){
	apiKey = apiKeyIn;
}

std::string TMDBService::buildSearchUrl(const std::string & query) const{
	return API_BASE + "/search/movie?api_key=" + apiKey + "&include_adult=false&query=" + urlEncode(query);
}

std::string TMDBService::buildDetailsUrl(int movieId) const{
	return API_BASE + "/movie/" + ofToString(movieId) + "?api_key=" + apiKey;
}

std::string TMDBService::buildPosterUrl(const std::string & posterPath, const std::string & size) const{
	return IMAGE_BASE + size + posterPath;
}

void TMDBService::searchMovies(const std::string & query){
	if(pendingSearchRequestId != -1){
		ofRemoveURLRequest(pendingSearchRequestId);
		pendingSearchRequestId = -1;
	}
	pendingSearchRequestId = ofLoadURLAsync(buildSearchUrl(query), "search");
}

void TMDBService::loadMovieDetails(std::shared_ptr<Movie> movie){
	if(!movie || movie->detailsLoaded){
		return;
	}
	int requestId = ofLoadURLAsync(buildDetailsUrl(movie->id), "details");
	pendingDetailRequests[requestId] = movie;
}

void TMDBService::loadPoster(std::shared_ptr<Movie> movie, const std::string & size){
	if(!movie || !movie->hasPoster() || movie->posterRequested){
		return;
	}
	movie->posterRequested = true;
	int requestId = ofLoadURLAsync(buildPosterUrl(movie->posterPath, size), "poster");
	pendingPosterRequests[requestId] = movie;
}

std::vector<std::shared_ptr<Movie>> TMDBService::parseSearchResults(const ofJson & json) const{
	std::vector<std::shared_ptr<Movie>> movies;
	if(json.contains("results") && json["results"].is_array()){
		for(const auto & item : json["results"]){
			auto movie = std::make_shared<Movie>();
			movie->id = item.value("id", 0);
			movie->title = item.value("title", std::string("Untitled"));
			movie->releaseDate = item.value("release_date", std::string(""));
			movie->overview = item.value("overview", std::string("No summary available for this film."));
			movie->posterPath = item.value("poster_path", std::string(""));
			movie->voteAverage = item.value("vote_average", 0.0);
			movies.push_back(movie);
		}
	}
	return movies;
}

void TMDBService::applyDetails(std::shared_ptr<Movie> movie, const ofJson & json) const{
	movie->tagline = json.value("tagline", std::string(""));
	movie->runtimeMinutes = json.value("runtime", 0);
	movie->genres.clear();
	if(json.contains("genres") && json["genres"].is_array()){
		for(const auto & genre : json["genres"]){
			movie->genres.push_back(genre.value("name", std::string("")));
		}
	}
	movie->detailsLoaded = true;
}

void TMDBService::urlResponse(ofHttpResponse & response){
	int requestId = response.request.getId();

	if(requestId == pendingSearchRequestId){
		pendingSearchRequestId = -1;
		if(response.status == 200){
			try{
				ofJson json = ofJson::parse(response.data.getText());
				auto movies = parseSearchResults(json);
				if(onSearchComplete) onSearchComplete(movies, true, "");
			}catch(std::exception & e){
				if(onSearchComplete) onSearchComplete({}, false, std::string("Couldn't understand the API response: ") + e.what());
			}
		}else{
			std::string message = "Search failed (status " + ofToString(response.status) + ")";
			if(response.status == 401){
				message = "Search failed: the TMDB API key is missing or invalid.";
			}
			if(onSearchComplete) onSearchComplete({}, false, message);
		}
		return;
	}

	auto detailIt = pendingDetailRequests.find(requestId);
	if(detailIt != pendingDetailRequests.end()){
		auto movie = detailIt->second;
		pendingDetailRequests.erase(detailIt);
		if(response.status == 200){
			try{
				ofJson json = ofJson::parse(response.data.getText());
				applyDetails(movie, json);
				if(onDetailsComplete) onDetailsComplete(movie, true, "");
			}catch(std::exception & e){
				if(onDetailsComplete) onDetailsComplete(movie, false, std::string("Couldn't parse film details: ") + e.what());
			}
		}else{
			if(onDetailsComplete) onDetailsComplete(movie, false, "Couldn't load extra details (status " + ofToString(response.status) + ")");
		}
		return;
	}

	auto posterIt = pendingPosterRequests.find(requestId);
	if(posterIt != pendingPosterRequests.end()){
		auto movie = posterIt->second;
		pendingPosterRequests.erase(posterIt);
		if(response.status == 200){
			if(ofLoadImage(movie->poster, response.data)){
				movie->posterLoaded = true;
			}
		}
		// A missing poster isn't fatal to the app - it just falls back
		// to the placeholder drawn in ofApp::drawResults/drawDetail.
		return;
	}
}
