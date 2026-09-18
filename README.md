# Film Finder

Repository: https://github.com/Ramizsrj/film-finder-app

A Data Driven App built with openFrameworks (C++) that searches [The Movie
Database (TMDB)](https://www.themoviedb.org/) for films and displays their
poster, release year, rating, genres, runtime and plot summary.

## Setup

1. Get a free TMDB API key: https://www.themoviedb.org/settings/api (choose
   "Developer" / API v3).
2. Open `src/Secrets.h` and replace `YOUR_TMDB_API_KEY_HERE` with your key.
   This file is gitignored so your key is never pushed to GitHub - a template
   is kept in `src/Secrets.example.h`.
3. Open `filmFinder.sln` in Visual Studio 2022 and build/run (Debug|x64 or
   Release|x64).

## Features

- Search films by title (type in the search bar, press Enter or click Search)
- Results grid with poster thumbnails, release year and rating
- Click a result for a detail view with tagline, genres, runtime and full
  plot summary
- Handles no-results, network errors and invalid/missing API key cases

## Structure

- `src/Movie.h/.cpp` - data model for a single film
- `src/TMDBService.h/.cpp` - wraps the TMDB REST API (async search, details
  and poster image requests)
- `src/ofApp.h/.cpp` - GUI, state machine (idle/searching/results/detail/
  error) and input handling
