const q = document.getElementById("q");
const go = document.getElementById("go");
const meta = document.getElementById("meta");
const suggestions = document.getElementById("suggestions");
const results = document.getElementById("results");

async function search() {
  const query = q.value.trim();
  if (!query) return;
  const res = await fetch("/search", {
    method: "POST",
    headers: {"Content-Type": "application/json"},
    body: JSON.stringify({query})
  });
  const data = await res.json();
  const mode = data.mode || "index_search";
  if (mode === "url_preview") {
    meta.textContent = `mode: URL Preview | backend: ${data.backend || "local"} | dedup: ${data.dedup ? "yes" : "no"} | doc_id: ${data.doc_id || "-"}`;
    results.innerHTML = "";
    const div = document.createElement("div");
    div.className = "card";
    div.innerHTML = `<div class="url">${data.url || ""}</div><div class="title">${data.title || data.url || "URL Preview"}</div><div class="snippet">${data.snippet || ""}</div>`;
    results.appendChild(div);
    return;
  }
  meta.textContent = `mode: Index Search | backend: ${data.backend || "local"} | latency: ${data.latency_us} us | cache: ${data.cache_hit ? "hit" : "miss"} | results: ${data.results.length}`;
  results.innerHTML = "";
  for (const r of data.results) {
    const div = document.createElement("div");
    div.className = "card";
    div.innerHTML = `<div class="url">${r.url || ("doc " + r.id)}</div><div class="title">${r.title}</div><div class="snippet">${r.snippet}</div>`;
    results.appendChild(div);
  }
}

async function updateSuggestions() {
  const query = q.value.trim();
  if (!query) {
    suggestions.innerHTML = "";
    return;
  }
  if (query.startsWith("http://") || query.startsWith("https://")) {
    suggestions.innerHTML = "";
    return;
  }
  const res = await fetch(`/suggest?q=${encodeURIComponent(query)}`);
  const data = await res.json();
  const values = Array.isArray(data) ? (data[1] || []) : (data.suggestions || []);
  suggestions.innerHTML = "";
  for (const s of values) {
    const li = document.createElement("li");
    li.textContent = s;
    li.onclick = () => { q.value = s; search(); };
    suggestions.appendChild(li);
  }
}

go.onclick = search;
q.addEventListener("keydown", (e) => { if (e.key === "Enter") search(); });
q.addEventListener("input", updateSuggestions);
