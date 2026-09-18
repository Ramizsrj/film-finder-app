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

	// ofJson::value(key, fallback) only substitutes the fallback when a key
	// is *missing* - TMDB frequently returns fields as explicit JSON null
	// (e.g. release_date, overview, poster_path on newer/incomplete
	// entries), which .value() throws on ("type must be string, but is
	// null"). These helpers treat missing-or-null the same way.
	std::string jsonString(const ofJson & json, const std::string & key, const std::string & fallback = ""){
		if(json.contains(key) && !json[key].is_null() && json[key].is_string()){
			return json[key].get<std::string>();
		}
		return fallback;
	}

	double jsonNumber(const ofJson & json, const std::string & key, double fallback = 0.0){
		if(json.contains(key) && !json[key].is_null() && json[key].is_number()){
			return json[key].get<double>();
		}
		return fallback;
	}

	int jsonInt(const ofJson & json, const std::string & key, int fallback = 0){
		if(json.contains(key) && !json[key].is_null() && json[key].is_number()){
			return json[key].get<int>();
		}
		return fallback;
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
			movie->id = jsonInt(item, "id");
			movie->title = jsonString(item, "title", "Untitled");
			movie->releaseDate = jsonString(item, "release_date");
			movie->overview = jsonString(item, "overview", "No summary available for this film.");
			movie->posterPath = jsonString(item, "poster_path");
			movie->voteAverage = jsonNumber(item, "vote_average");
			movies.push_back(movie);
		}
	}
	return movies;
}

void TMDBService::applyDetails(std::shared_ptr<Movie> movie, const ofJson & json) const{
	movie->tagline = jsonString(json, "tagline");
	movie->runtimeMinutes = jsonInt(json, "runtime");
	movie->genres.clear();
	if(json.contains("genres") && json["genres"].is_array()){
		for(const auto & genre : json["genres"]){
			movie->genres.push_back(jsonString(genre, "name"));
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
				// ofLoadImage(ofImage&, ofBuffer) decodes via ofImage's
				// implicit conversion to ofPixels&, which fills the pixel
				// data but does NOT touch ofImage's own width/height/texture
				// bookkeeping - update() copies pixels -> texture and syncs
				// getWidth()/getHeight(), which draw() and our layout code
				// both rely on.
				movie->poster.update();
				movie->posterLoaded = true;
			}
		}
		// A missing poster isn't fatal to the app - it just falls back
		// to the placeholder drawn in ofApp::drawResults/drawDetail.
		return;
	}
}
