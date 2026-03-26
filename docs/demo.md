# Demo Script (OpenSearch URL Search MVP)

## 1) Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## 2) Validate Core
```bash
ctest --test-dir build --output-on-failure
./build/bench_engine data/sample_wiki.tsv
./build/api_bench
```

## 3) Start OpenSearch
```bash
docker compose up -d
```

## 4) Crawl and Create Snapshot
```bash
./build/crawl_index_tool https://example.com 3
```

## 5) Start API + Web
```bash
./build/search_api 8080
```
Open: `http://localhost:8080`

## 6) API Smoke Test
```bash
curl -s http://localhost:8080/health
curl -s -X POST http://localhost:8080/search -H 'Content-Type: application/json' -d '{"query":"example domain"}'
curl -s 'http://localhost:8080/suggest?q=exa'
curl -s 'http://localhost:8080/doc?id=1'
```

## 7) Full Smoke Script
```bash
./scripts/smoke_opensearch.sh 8080
./scripts/smoke_browser_endpoints.sh 8080
```

## 8) Optional TUI Demo
```bash
./build/search_cli data/sample_wiki.tsv
```

## 9) Chromium Omnibox Setup
- Chromium settings -> search engines.
- Add custom search:
  - Name: `Google Lite`
  - Shortcut: `gl`
  - URL: `http://localhost:8080/search?q=%s&page=1&size=10`
- Type `gl` then press space in address bar and run queries.
