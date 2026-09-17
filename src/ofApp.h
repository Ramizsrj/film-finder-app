#pragma once

#include "ofMain.h"
#include "TMDBService.h"

// Drives the whole screen: which panel is showing, and what happens
// while search results (or an error) are waiting to arrive.
enum class AppState{
	Idle,
	Searching,
	Results,
	Detail,
	Error
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

	private:
		// --- layout (computed on demand from the current window size, so
		// the same numbers are used for both drawing and hit-testing) ---
		ofRectangle getSearchBoxBounds() const;
		ofRectangle getSearchButtonBounds() const;
		ofRectangle getBackButtonBounds() const;
		std::vector<ofRectangle> getResultCardBounds() const;

		// --- drawing ---
		void drawSearchBar();
		void drawStatusBanner();
		void drawResults();
		void drawDetail();
		void drawMovieCard(const ofRectangle & bounds, const std::shared_ptr<Movie> & movie);
		std::vector<std::string> wrapText(ofTrueTypeFont & font, const std::string & text, float maxWidth) const;

		// --- actions ---
		void submitSearch();
		void selectMovie(std::shared_ptr<Movie> movie);
		void goBackToResults();

		// --- TMDBService callbacks ---
		void handleSearchComplete(std::vector<std::shared_ptr<Movie>> movies, bool success, std::string errorMessage);
		void handleDetailsComplete(std::shared_ptr<Movie> movie, bool success, std::string errorMessage);

		TMDBService tmdb;
		AppState state = AppState::Idle;

		std::string searchQuery;
		std::vector<std::shared_ptr<Movie>> searchResults;
		std::shared_ptr<Movie> selectedMovie;

		std::string statusMessage;
		bool statusIsError = false;

		ofTrueTypeFont titleFont;
		ofTrueTypeFont bodyFont;
		ofTrueTypeFont smallFont;

		static const int CARD_WIDTH = 200;
		static const int CARD_HEIGHT = 320;
		static const int CARD_MARGIN = 20;
		static const int POSTER_HEIGHT = 220;
};
