#include "Movie.h"

std::string Movie::getReleaseYear() const{
	if(releaseDate.size() >= 4){
		return releaseDate.substr(0, 4);
	}
	return "Unknown";
}

std::string Movie::getGenresSummary() const{
	if(genres.empty()){
		return "";
	}
	std::string summary;
	for(size_t i = 0; i < genres.size(); i++){
		summary += genres[i];
		if(i + 1 < genres.size()){
			summary += ", ";
		}
	}
	return summary;
}

bool Movie::hasPoster() const{
	return !posterPath.empty();
}
