# Google Lite v2: OpenSearch URL Search MVP

Single-node, Google-like search platform in C++ with OpenSearch-backed search, URL preview ingestion, local fallback index, HTTP API, and polished browser UI.

## What It Does
- Crawls pages from seed URLs (`curl`-based fetch + HTML text/link extraction)
- Stores crawled docs and frontier state on disk (`data/store/`)
- Rebuilds/loads index snapshots from persisted documents
- Serves search over HTTP:
  - `POST /search`
  - `GET /suggest?q=...`
  - `GET /doc?id=...`
  - `GET /health`
- Hosts a browser UI (`/`) with suggestions, results, and latency/cache display
- Uses OpenSearch backend when available (`http://localhost:9200`), otherwise local fallback

## System Modules
- `src/crawl`: crawler and HTML extraction
- `src/io`: TSV loader, doc store, parallel builder
- `src/index`: in-memory index + snapshot persistence flow
- `src/search`: query service + LRU cache
- `src/api`: lightweight HTTP server
- `src/app`:
  - `crawl_index_tool` for ingestion/indexing
  - `search_api` for serving web/API
  - existing `search_cli` TUI stays available

## Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Start OpenSearch (for full MVP path)
```bash
docker compose up -d
```

## Run Crawler + Build Snapshot
```bash
./build/crawl_index_tool https://example.com 3
```

## Run API + Web UI
```bash
./build/search_api 8080
```
- Open `http://localhost:8080`
- API examples:
  - `curl -s http://localhost:8080/health`
  - `curl -s -X POST http://localhost:8080/search -H 'Content-Type: application/json' -d '{"query":"example domain"}'`
  - `curl -s 'http://localhost:8080/suggest?q=exa'`
  - `curl -s 'http://localhost:8080/search?q=example+domain&page=1&size=10'`

## OpenSearch Smoke Script
```bash
./scripts/smoke_opensearch.sh 8080
./scripts/smoke_browser_endpoints.sh 8080
```

## Chromium Integration (Address Bar Search)
1. Start services and API:
   - `docker compose up -d`
   - `./build/search_api 8080`
2. Open Chromium `Settings -> Search engine -> Manage search engines and site search`.
3. Add site search entry:
   - **Name**: `Google Lite`
   - **Shortcut**: `gl`
   - **URL with %s**: `http://localhost:8080/search?q=%s&page=1&size=10`
4. Set it as default (or type `gl <space> query` in omnibox).
5. Optional: visit `http://localhost:8080/opensearch.xml` to expose descriptor metadata.

## Optional TUI (existing)
```bash
./build/search_cli data/sample_wiki.tsv
```

## Tests + Benchmarks
```bash
ctest --test-dir build --output-on-failure
./build/bench_engine data/sample_wiki.tsv
./build/api_bench
```

## Current Measured Output
- Crawl/index smoke run:
  - `Crawled docs=1 indexed_docs=1 vocab=13`
- API bench:
  - `api_p50_us=19`
  - `api_p95_us=19`
- Search bench (sample corpus):
  - `compressed_bytes=172`
  - `p50_latency_us=12`
  - `p95_latency_us=12`

## Resume Bullets
- Built a single-node web search platform in C++ with crawl->store->index->serve architecture and a browser-based query UI.
- Implemented persistent document store and index snapshot reload flow for resumable ingestion and restart-safe serving.
- Exposed low-latency REST endpoints for search/suggest/doc retrieval with cache-aware query execution and benchmarked p50/p95 performance.
