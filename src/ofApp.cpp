#include "ofApp.h"
#include "Secrets.h"
#include <sstream>

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetWindowTitle("Film Finder");
	ofBackground(24, 26, 32);
	ofSetVerticalSync(true);
	ofEnableAlphaBlending();

	titleFont.load(OF_TTF_SANS, 30);
	bodyFont.load(OF_TTF_SANS, 16);
	smallFont.load(OF_TTF_SANS, 13);

	tmdb.setup(TMDB_API_KEY);
	tmdb.onSearchComplete = [this](std::vector<std::shared_ptr<Movie>> movies, bool success, std::string errorMessage){
		handleSearchComplete(movies, success, errorMessage);
	};
	tmdb.onDetailsComplete = [this](std::shared_ptr<Movie> movie, bool success, std::string errorMessage){
		handleDetailsComplete(movie, success, errorMessage);
	};

	if(TMDB_API_KEY == "YOUR_TMDB_READ_ACCESS_TOKEN_HERE"){
		statusMessage = "Add your TMDB API key to src/Secrets.h before searching.";
		statusIsError = true;
	}else{
		statusMessage = "Type a film title and press Enter to search.";
		statusIsError = false;
	}
}

//--------------------------------------------------------------
void ofApp::update(){
	// Responses from TMDBService arrive via ofURLFileLoader's own update
	// listener and are delivered through the onSearchComplete /
	// onDetailsComplete callbacks - nothing to poll here.
}

//--------------------------------------------------------------
void ofApp::draw(){
	drawSearchBar();
	drawStatusBanner();

	switch(state){
		case AppState::Results:
			drawResults();
			break;
		case AppState::Detail:
			drawDetail();
			break;
		default:
			break;
	}
}

//--------------------------------------------------------------
ofRectangle ofApp::getSearchBoxBounds() const{
	float margin = 40;
	float buttonWidth = 110;
	float width = std::max(200.0f, ofGetWidth() - margin * 2 - buttonWidth - 10);
	return ofRectangle(margin, 30, width, 44);
}

//--------------------------------------------------------------
ofRectangle ofApp::getSearchButtonBounds() const{
	ofRectangle box = getSearchBoxBounds();
	return ofRectangle(box.getRight() + 10, box.y, 110, 44);
}

//--------------------------------------------------------------
ofRectangle ofApp::getBackButtonBounds() const{
	return ofRectangle(40, 30, 100, 40);
}

//--------------------------------------------------------------
std::vector<ofRectangle> ofApp::getResultCardBounds() const{
	std::vector<ofRectangle> bounds;
	float margin = 40;
	float top = 110;
	float availableWidth = ofGetWidth() - margin * 2;
	int columns = std::max(1, int((availableWidth + CARD_MARGIN) / (CARD_WIDTH + CARD_MARGIN)));

	for(size_t i = 0; i < searchResults.size(); i++){
		int col = static_cast<int>(i) % columns;
		int row = static_cast<int>(i) / columns;
		float x = margin + col * (CARD_WIDTH + CARD_MARGIN);
		float y = top + row * (CARD_HEIGHT + CARD_MARGIN);
		bounds.emplace_back(x, y, CARD_WIDTH, CARD_HEIGHT);
	}
	return bounds;
}

//--------------------------------------------------------------
void ofApp::drawSearchBar(){
	ofRectangle box = getSearchBoxBounds();
	ofSetColor(40, 43, 52);
	ofDrawRectRounded(box, 6);

	bool showCursor = (int(ofGetElapsedTimef() * 2) % 2) == 0;
	if(searchQuery.empty()){
		ofSetColor(120, 124, 134);
		bodyFont.drawString("Search for a film...", box.x + 16, box.y + box.height / 2 + 6);
	}else{
		ofSetColor(230);
		std::string display = showCursor ? searchQuery + "|" : searchQuery;
		bodyFont.drawString(display, box.x + 16, box.y + box.height / 2 + 6);
	}

	ofRectangle button = getSearchButtonBounds();
	ofSetColor(70, 130, 220);
	ofDrawRectRounded(button, 6);
	ofSetColor(255);
	float textWidth = bodyFont.stringWidth("Search");
	bodyFont.drawString("Search", button.x + (button.width - textWidth) / 2, button.y + button.height / 2 + 6);
}

//--------------------------------------------------------------
void ofApp::drawStatusBanner(){
	if(statusMessage.empty()){
		return;
	}
	ofSetColor(statusIsError ? ofColor(210, 100, 100) : ofColor(160, 170, 185));
	smallFont.drawString(statusMessage, 40, 95);
	ofSetColor(255);
}

//--------------------------------------------------------------
void ofApp::drawMovieCard(const ofRectangle & bounds, const std::shared_ptr<Movie> & movie){
	ofSetColor(38, 41, 50);
	ofDrawRectangle(bounds);

	ofRectangle posterArea(bounds.x, bounds.y, bounds.width, POSTER_HEIGHT);
	if(movie->posterLoaded && movie->poster.isAllocated()){
		ofSetColor(255);
		movie->poster.draw(posterArea);
	}else{
		ofSetColor(58, 61, 70);
		ofDrawRectangle(posterArea);
		ofSetColor(140);
		std::string label = movie->hasPoster() ? "Loading..." : "No poster";
		float w = smallFont.stringWidth(label);
		smallFont.drawString(label, posterArea.x + (posterArea.width - w) / 2, posterArea.y + posterArea.height / 2);
	}

	ofSetColor(230);
	float textY = bounds.y + POSTER_HEIGHT + 22;
	auto titleLines = wrapText(bodyFont, movie->title, bounds.width - 16);
	for(size_t i = 0; i < titleLines.size() && i < 2; i++){
		bodyFont.drawString(titleLines[i], bounds.x + 8, textY);
		textY += 20;
	}

	ofSetColor(150);
	std::string meta = movie->getReleaseYear() + "  |  " + ofToString(movie->voteAverage, 1) + "/10";
	smallFont.drawString(meta, bounds.x + 8, bounds.y + bounds.height - 12);
	ofSetColor(255);
}

//--------------------------------------------------------------
void ofApp::drawResults(){
	auto bounds = getResultCardBounds();
	for(size_t i = 0; i < searchResults.size(); i++){
		drawMovieCard(bounds[i], searchResults[i]);
	}
}

//--------------------------------------------------------------
void ofApp::drawDetail(){
	if(!selectedMovie){
		return;
	}

	ofRectangle back = getBackButtonBounds();
	ofSetColor(50, 53, 62);
	ofDrawRectRounded(back, 6);
	ofSetColor(230);
	smallFont.drawString("< Back", back.x + 16, back.y + back.height / 2 + 5);

	float top = 100;
	ofRectangle posterArea(40, top, 220, 320);
	if(selectedMovie->posterLoaded && selectedMovie->poster.isAllocated()){
		ofSetColor(255);
		selectedMovie->poster.draw(posterArea);
	}else{
		ofSetColor(58, 61, 70);
		ofDrawRectangle(posterArea);
		ofSetColor(140);
		std::string label = selectedMovie->hasPoster() ? "Loading..." : "No poster";
		float w = smallFont.stringWidth(label);
		smallFont.drawString(label, posterArea.x + (posterArea.width - w) / 2, posterArea.y + posterArea.height / 2);
	}

	float textX = posterArea.getRight() + 30;
	float textWidth = ofGetWidth() - textX - 40;
	float y = top + 30;

	ofSetColor(255);
	auto titleLines = wrapText(titleFont, selectedMovie->title, textWidth);
	for(auto & line : titleLines){
		titleFont.drawString(line, textX, y);
		y += 34;
	}

	y += 6;
	ofSetColor(150);
	std::string meta = selectedMovie->getReleaseYear() + "  |  Rating " + ofToString(selectedMovie->voteAverage, 1) + "/10";
	if(selectedMovie->detailsLoaded && selectedMovie->runtimeMinutes > 0){
		meta += "  |  " + ofToString(selectedMovie->runtimeMinutes) + " min";
	}
	bodyFont.drawString(meta, textX, y);
	y += 30;

	if(selectedMovie->detailsLoaded && !selectedMovie->getGenresSummary().empty()){
		ofSetColor(120, 170, 220);
		smallFont.drawString(selectedMovie->getGenresSummary(), textX, y);
		y += 26;
	}else if(!selectedMovie->detailsLoaded){
		ofSetColor(130);
		smallFont.drawString("Loading more details...", textX, y);
		y += 26;
	}

	if(selectedMovie->detailsLoaded && !selectedMovie->tagline.empty()){
		ofSetColor(150, 150, 165);
		auto taglineLines = wrapText(smallFont, "\"" + selectedMovie->tagline + "\"", textWidth);
		for(auto & line : taglineLines){
			smallFont.drawString(line, textX, y);
			y += 18;
		}
		y += 10;
	}

	ofSetColor(220);
	auto overviewLines = wrapText(bodyFont, selectedMovie->overview, textWidth);
	for(auto & line : overviewLines){
		bodyFont.drawString(line, textX, y);
		y += 22;
	}

	ofSetColor(255);
}

//--------------------------------------------------------------
std::vector<std::string> ofApp::wrapText(ofTrueTypeFont & font, const std::string & text, float maxWidth) const{
	std::vector<std::string> lines;
	std::istringstream words(text);
	std::string word;
	std::string currentLine;

	while(words >> word){
		std::string candidate = currentLine.empty() ? word : currentLine + " " + word;
		if(font.stringWidth(candidate) > maxWidth && !currentLine.empty()){
			lines.push_back(currentLine);
			currentLine = word;
		}else{
			currentLine = candidate;
		}
	}
	if(!currentLine.empty()){
		lines.push_back(currentLine);
	}
	if(lines.empty()){
		lines.push_back("");
	}
	return lines;
}

//--------------------------------------------------------------
void ofApp::submitSearch(){
	if(TMDB_API_KEY == "YOUR_TMDB_READ_ACCESS_TOKEN_HERE"){
		statusMessage = "Add your TMDB API key to src/Secrets.h before searching.";
		statusIsError = true;
		return;
	}
	if(searchQuery.empty()){
		statusMessage = "Type a film title first.";
		statusIsError = true;
		return;
	}

	state = AppState::Searching;
	statusMessage = "Searching for \"" + searchQuery + "\"...";
	statusIsError = false;
	searchResults.clear();
	selectedMovie.reset();
	tmdb.searchMovies(searchQuery);
}

//--------------------------------------------------------------
void ofApp::selectMovie(std::shared_ptr<Movie> movie){
	if(!movie){
		return;
	}
	selectedMovie = movie;
	state = AppState::Detail;
	if(!movie->detailsLoaded){
		tmdb.loadMovieDetails(movie);
	}
}

//--------------------------------------------------------------
void ofApp::goBackToResults(){
	selectedMovie.reset();
	state = searchResults.empty() ? AppState::Idle : AppState::Results;
}

//--------------------------------------------------------------
void ofApp::handleSearchComplete(std::vector<std::shared_ptr<Movie>> movies, bool success, std::string errorMessage){
	if(!success){
		state = AppState::Error;
		statusIsError = true;
		statusMessage = errorMessage.empty() ? "Something went wrong contacting TMDB." : errorMessage;
		return;
	}

	searchResults = movies;

	if(searchResults.empty()){
		state = AppState::Error;
		statusIsError = true;
		statusMessage = "No films found for \"" + searchQuery + "\". Try another title.";
		return;
	}

	state = AppState::Results;
	statusIsError = false;
	statusMessage = "Found " + ofToString(searchResults.size()) + " result(s) for \"" + searchQuery + "\".";

	for(auto & movie : searchResults){
		tmdb.loadPoster(movie);
	}
}

//--------------------------------------------------------------
void ofApp::handleDetailsComplete(std::shared_ptr<Movie> movie, bool success, std::string errorMessage){
	if(!success){
		ofLogWarning("ofApp") << "Extra details failed to load: " << errorMessage;
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if(key == OF_KEY_RETURN){
		submitSearch();
		return;
	}
	if(key == OF_KEY_BACKSPACE){
		if(!searchQuery.empty()){
			searchQuery.pop_back();
		}
		return;
	}
	if(key == OF_KEY_ESC){
		if(state == AppState::Detail){
			goBackToResults();
		}
		return;
	}
	if(key >= 32 && key <= 126 && searchQuery.size() < 100){
		searchQuery += static_cast<char>(key);
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	if(getSearchButtonBounds().inside(x, y)){
		submitSearch();
		return;
	}

	if(state == AppState::Detail){
		if(getBackButtonBounds().inside(x, y)){
			goBackToResults();
		}
		return;
	}

	if(state == AppState::Results){
		auto bounds = getResultCardBounds();
		for(size_t i = 0; i < bounds.size(); i++){
			if(bounds[i].inside(x, y)){
				selectMovie(searchResults[i]);
				return;
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
