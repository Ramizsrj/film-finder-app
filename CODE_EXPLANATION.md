# Code Explanation

This document explains how Film Finder is structured and the key
programming techniques used, for reference alongside the source code.

## Architecture overview

The app is split into three parts, each with a single responsibility:

- **`Movie`** (`src/Movie.h/.cpp`) - a plain data model for one film. Holds
  the fields returned by TMDB (title, release date, overview, poster path,
  rating) plus fields populated later by a details lookup (tagline,
  runtime, genres) and by the async poster download (an `ofImage` and two
  boolean flags tracking its load state).

- **`TMDBService`** (`src/TMDBService.h/.cpp`) - all networking and JSON
  parsing. `ofApp` never builds a URL or touches `ofJson` directly; it just
  calls `searchMovies()`, `loadMovieDetails()` and `loadPoster()` and
  receives results through callbacks. This keeps the GUI code free of HTTP
  and parsing details, and means the network layer could be swapped or
  unit-tested independently.

- **`ofApp`** (`src/ofApp.h/.cpp`) - the GUI: layout, drawing, input
  handling and the state machine that decides what's on screen.

## Key techniques

### Asynchronous networking with a callback interface

TMDB requests use `ofLoadURLAsync` / `ofURLFileLoader::handleRequestAsync`
rather than the blocking `ofLoadURL`, so the GUI keeps redrawing (and the
search bar cursor keeps blinking) while a request is in flight. openFrameworks
delivers every completed request through one global event
(`ofURLResponseEvent`); `TMDBService::urlResponse()` is registered as its
listener via `ofRegisterURLNotification(this)` and routes each response to
the right handler by comparing `response.request.getId()` against the ids
it is tracking (one for the current search, and `std::map<int,
shared_ptr<Movie>>` for in-flight detail/poster requests keyed by request
id).

`TMDBService` exposes this to `ofApp` as two `std::function` members
(`onSearchComplete`, `onDetailsComplete`) rather than a fixed virtual
interface, so `ofApp::setup()` can wire them up with simple lambdas:

```cpp
tmdb.onSearchComplete = [this](std::vector<std::shared_ptr<Movie>> movies, bool success, std::string errorMessage){
    handleSearchComplete(movies, success, errorMessage);
};
```

### Authentication via HTTP header

TMDB's v4 Read Access Token is a JWT that must be sent as an
`Authorization: Bearer <token>` header, not a query parameter. Since the
free-function `ofLoadURLAsync` doesn't expose custom headers,
`TMDBService` keeps its own `ofURLFileLoader` instance and builds a full
`ofHttpRequest` (`buildAuthorizedRequest()`) with the header set, then
calls `loader.handleRequestAsync(request)`. Poster images are public, so
those still use the simpler `ofLoadURLAsync`.

### JSON parsing

Responses are parsed with `ofJson` (openFrameworks' bundled
`nlohmann::json`). `TMDBService::parseSearchResults()` and `::applyDetails()`
use `json.value(key, fallback)` throughout, so a missing or unexpected
field (e.g. a film with no overview) falls back to a sane default instead
of throwing.

### A small state machine

`AppState` (`Idle`, `Searching`, `Results`, `Detail`, `Error`) drives what
`ofApp::draw()` shows and what `mousePressed`/`keyPressed` do. Centralising
this in one enum, rather than a handful of independent booleans, makes it
straightforward to reason about (e.g. "can a card be clicked?" is simply
"is state == Results?").

### Deterministic, recomputed layout

Card/button positions (`getSearchBoxBounds()`, `getResultCardBounds()`,
etc.) are pure functions of the current window size and result count,
computed fresh each time they're needed rather than cached. `draw()` and
`mousePressed()` both call the same functions, so hit-testing always
matches what was actually drawn - there's no separate layout cache that
could go stale after a resize.

### Manual text wrapping

`ofTrueTypeFont` doesn't wrap text itself, so `ofApp::wrapText()` greedily
packs words onto a line while `font.stringWidth(candidate)` stays under
the available width, used for film titles, taglines and plot summaries.

### Error handling

Three distinct failure cases are handled and shown to the user rather than
silently ignored: an HTTP/network failure, a successful response with zero
results, and a missing/placeholder API key (checked once in `setup()` and
again in `submitSearch()`).

## Where to look for what

| Feature | File |
|---|---|
| Search bar + text input | `ofApp::drawSearchBar`, `ofApp::keyPressed` |
| Results grid + poster placeholders | `ofApp::drawResults`, `ofApp::drawMovieCard` |
| Detail view | `ofApp::drawDetail` |
| TMDB search/details/poster requests | `TMDBService.cpp` |
| URL building + percent-encoding | `TMDBService::buildSearchUrl`, anonymous `urlEncode()` |
