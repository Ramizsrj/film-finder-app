#pragma once

#include "ofMain.h"
#include "Movie.h"

// Thin wrapper around the TMDB (The Movie Database) REST API.
// https://developer.themoviedb.org/reference/search-movie
//
// All requests are asynchronous (ofLoadURLAsync) so the GUI never blocks
// while waiting on the network. Results are delivered through the
// onSearchComplete / onDetailsComplete callbacks, which ofApp wires up
// to its own handlers in setup().
class TMDBService{

	public:
		TMDBService();
		~TMDBService();

		void setup(const std::string & apiKeyIn);

		// Starts an async search; any previous in-flight search is cancelled.
		void searchMovies(const std::string & query);

		// Starts an async lookup of extended details (runtime, genres, tagline)
		// for a movie already returned by a search.
		void loadMovieDetails(std::shared_ptr<Movie> movie);

		// Starts an async poster image download for a movie, if it has one
		// and it hasn't already been requested.
		void loadPoster(std::shared_ptr<Movie> movie, const std::string & size = "w342");

		std::function<void(std::vector<std::shared_ptr<Movie>>, bool, std::string)> onSearchComplete;
		std::function<void(std::shared_ptr<Movie>, bool, std::string)> onDetailsComplete;

		// Registered with ofRegisterURLNotification; routes every completed
		// HTTP request to the right handler based on its request id.
		void urlResponse(ofHttpResponse & response);

	private:
		std::string buildSearchUrl(const std::string & query) const;
		std::string buildDetailsUrl(int movieId) const;
		std::string buildPosterUrl(const std::string & posterPath, const std::string & size) const;

		std::vector<std::shared_ptr<Movie>> parseSearchResults(const ofJson & json) const;
		void applyDetails(std::shared_ptr<Movie> movie, const ofJson & json) const;

		std::string apiKey;

		int pendingSearchRequestId = -1;
		std::map<int, std::shared_ptr<Movie>> pendingDetailRequests;
		std::map<int, std::shared_ptr<Movie>> pendingPosterRequests;

		static const std::string API_BASE;
		static const std::string IMAGE_BASE;
};
