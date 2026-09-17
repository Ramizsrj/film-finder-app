#pragma once

#include "ofMain.h"

// Represents a single film returned by the TMDB API, plus any extra
// detail/poster data loaded lazily once the user selects it.
class Movie{

	public:
		int id = 0;
		std::string title;
		std::string releaseDate;
		std::string overview;
		std::string posterPath;
		double voteAverage = 0.0;

		// Populated only after a successful "details" lookup.
		std::string tagline;
		int runtimeMinutes = 0;
		std::vector<std::string> genres;
		bool detailsLoaded = false;

		ofImage poster;
		bool posterRequested = false;
		bool posterLoaded = false;

		std::string getReleaseYear() const;
		std::string getGenresSummary() const;
		bool hasPoster() const;
};
