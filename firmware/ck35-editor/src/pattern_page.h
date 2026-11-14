#pragma once

const char pattern_index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>CK-35 editor</title>
  <style>
    body { font-family: sans-serif; margin: 16px; background: #f4f1ea; color: #1c1a17; }
    h1 { font-size: 1.25rem; margin: 0 0 8px; }
    .note { background: #fff8e8; border: 1px solid #e0c98a; padding: 8px 10px; max-width: 52rem; }
    .row { display: flex; flex-wrap: wrap; gap: 12px; align-items: end; margin: 12px 0; }
    label { display: flex; flex-direction: column; font-size: 0.85rem; gap: 4px; }
    input, select, textarea, button { font: inherit; }
    textarea { width: 16rem; height: 3.2rem; }
    canvas { border: 1px solid #222; background: #fff; touch-action: none; image-rendering: pixelated; max-width: 100%; }
    button { padding: 6px 10px; }
    #status { min-height: 1.2em; }
    #list button { margin: 0 6px 6px 0; }
  </style>
</head>
<body>
  <h1>Редактор узора CK-35</h1>
  <p class="note">Черновик. В картридж ничего не записывается: на флеш ESP32 уходит только JSON этой страницы. Кодировщик Brother Pattern Memory ещё не написан, байтовый формат CK-35 не подтверждён.</p>
  <div class="row">
    <label>id файла<input id="pid" value="pattern1" maxlength="24"></label>
    <label>имя<input id="pname" value=""></label>
    <label>петли<input id="pw" type="number" min="1" max="96" value="24"></label>
    <label>ряды<input id="ph" type="number" min="1" max="96" value="32"></label>
    <label>режим
      <select id="pmode">
        <option value="">не задан</option>
        <option value="0">0 other</option>
        <option value="1">1 SB jacquard</option>
        <option value="2">2 DB jacquard, 2 цвета</option>
        <option value="3">3 DB jacquard, 3 цвета</option>
        <option value="4">4 DB jacquard, 4 цвета</option>
      </select>
    </label>
    <label>цветов
      <select id="pcount">
        <option value="2" selected>2</option>
        <option value="3">3</option>
        <option value="4">4</option>
      </select>
    </label>
  </div>
  <div class="row" id="swatches"></div>
  <div class="row">
    <label>номер блока<input id="bnum" type="number" value="1"></label>
    <label>Block<textarea id="bblock"></textarea></label>
    <label>Holder<textarea id="bholder"></textarea></label>
    <label>Ranges<textarea id="branges"></textarea></label>
    <label>Stops<textarea id="bstops"></textarea></label>
  </div>
  <div class="row">
    <button type="button" id="apply">Применить размер</button>
    <button type="button" id="save">Сохранить JSON</button>
    <label>PNG<input id="png" type="file" accept="image/png,image/*"></label>
  </div>
  <p id="status"></p>
  <canvas id="grid"></canvas>
  <h2>Сохранённые</h2>
  <div id="list"></div>
<script>
const cell = 14;
const canvas = document.getElementById("grid");
const ctx = canvas.getContext("2d");
const state = blank(24, 32);
let brush = 1;
let drawing = false;

function blank(w, h) {
  const rows = [];
  for (let y = 0; y < h; y++) rows.push("0".repeat(w));
  return {
    version: 1, id: "pattern1", name: "", mode: null,
    colors: ["#111111", "#f7f4ea", "#c0392b", "#2980b9"],
    colorCount: 2, width: w, height: h, rows: rows,
    block: { number: 1, block: "", holder: "", ranges: "", stops: "" }
  };
}
function $(id) { return document.getElementById(id); }
function setStatus(t) { $("status").textContent = t; }
function clamp(n, a, b) { n = Number(n); if (!Number.isFinite(n)) n = a; return Math.max(a, Math.min(b, n)); }
function hexToRgb(hex) {
  const h = hex.replace("#", "");
  return { r: parseInt(h.slice(0, 2), 16), g: parseInt(h.slice(2, 4), 16), b: parseInt(h.slice(4, 6), 16) };
}
function renderSwatches() {
  const box = $("swatches");
  box.innerHTML = "";
  for (let i = 0; i < 4; i++) {
    const label = document.createElement("label");
    label.textContent = "цвет " + i;
    const input = document.createElement("input");
    input.type = "color";
    input.value = state.colors[i];
    input.disabled = i >= state.colorCount;
    input.addEventListener("input", function () { state.colors[i] = input.value; draw(); });
    const pick = document.createElement("button");
    pick.type = "button";
    pick.textContent = brush === i ? "кисть" : "взять";
    pick.disabled = i >= state.colorCount;
    pick.addEventListener("click", function () { brush = i; renderSwatches(); });
    label.appendChild(input);
    label.appendChild(pick);
    box.appendChild(label);
  }
}
function draw() {
  canvas.width = state.width * cell;
  canvas.height = state.height * cell;
  for (let y = 0; y < state.height; y++) {
    const row = state.rows[y] || "";
    for (let x = 0; x < state.width; x++) {
      const idx = clamp(row.charCodeAt(x) - 48, 0, state.colorCount - 1);
      ctx.fillStyle = state.colors[idx] || "#fff";
      ctx.fillRect(x * cell, y * cell, cell, cell);
      ctx.strokeStyle = "rgba(0,0,0,0.15)";
      ctx.strokeRect(x * cell + 0.5, y * cell + 0.5, cell - 1, cell - 1);
    }
  }
}
function paintAt(ev) {
  const r = canvas.getBoundingClientRect();
  const x = Math.floor((ev.clientX - r.left) * canvas.width / r.width / cell);
  const y = Math.floor((ev.clientY - r.top) * canvas.height / r.height / cell);
  if (x < 0 || y < 0 || x >= state.width || y >= state.height) return;
  const row = state.rows[y].split("");
  row[x] = String(brush);
  state.rows[y] = row.join("");
  draw();
}
canvas.addEventListener("pointerdown", function (ev) { drawing = true; canvas.setPointerCapture(ev.pointerId); paintAt(ev); });
canvas.addEventListener("pointermove", function (ev) { if (drawing) paintAt(ev); });
canvas.addEventListener("pointerup", function () { drawing = false; });
canvas.addEventListener("pointercancel", function () { drawing = false; });

function readForm() {
  state.id = $("pid").value.trim();
  state.name = $("pname").value;
  state.colorCount = clamp($("pcount").value, 2, 4);
  const mode = $("pmode").value;
  state.mode = mode === "" ? null : Number(mode);
  state.block = {
    number: Number($("bnum").value),
    block: $("bblock").value,
    holder: $("bholder").value,
    ranges: $("branges").value,
    stops: $("bstops").value
  };
}
function writeForm() {
  $("pid").value = state.id || "pattern1";
  $("pname").value = state.name || "";
  $("pw").value = state.width;
  $("ph").value = state.height;
  $("pcount").value = String(state.colorCount || 2);
  $("pmode").value = state.mode === null || state.mode === undefined ? "" : String(state.mode);
  $("bnum").value = state.block && state.block.number !== undefined ? state.block.number : 1;
  $("bblock").value = (state.block && state.block.block) || "";
  $("bholder").value = (state.block && state.block.holder) || "";
  $("branges").value = (state.block && state.block.ranges) || "";
  $("bstops").value = (state.block && state.block.stops) || "";
  renderSwatches();
  draw();
}
function applySize() {
  const w = clamp($("pw").value, 1, 96);
  const h = clamp($("ph").value, 1, 96);
  const rows = [];
  for (let y = 0; y < h; y++) {
    let row = "";
    for (let x = 0; x < w; x++) {
      row += (y < state.height && state.rows[y] && x < state.rows[y].length) ? state.rows[y][x] : "0";
    }
    rows.push(row);
  }
  state.width = w;
  state.height = h;
  state.rows = rows;
  draw();
}
$("apply").addEventListener("click", applySize);
$("pcount").addEventListener("change", function () {
  state.colorCount = clamp($("pcount").value, 2, 4);
  if (brush >= state.colorCount) brush = 0;
  renderSwatches();
  draw();
});
$("save").addEventListener("click", function () {
  readForm();
  if (!/^[A-Za-z0-9_-]{1,24}$/.test(state.id)) {
    setStatus("id: латиница, цифры, _ и -, до 24 знаков");
    return;
  }
  fetch("/api/pattern", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(state)
  }).then(function (res) {
    if (!res.ok) return res.text().then(function (t) { throw new Error(t || res.status); });
    setStatus("сохранено /patterns/" + state.id + ".json");
    refreshList();
  }).catch(function (err) { setStatus(String(err.message || err)); });
});
$("png").addEventListener("change", function () {
  const file = $("png").files && $("png").files[0];
  if (!file) return;
  readForm();
  const img = new Image();
  img.onload = function () {
    const c = document.createElement("canvas");
    c.width = state.width;
    c.height = state.height;
    const g = c.getContext("2d");
    g.drawImage(img, 0, 0, state.width, state.height);
    const px = g.getImageData(0, 0, state.width, state.height).data;
    const rows = [];
    for (let y = 0; y < state.height; y++) {
      let row = "";
      for (let x = 0; x < state.width; x++) {
        const o = (y * state.width + x) * 4;
        if (px[o + 3] < 16) { row += "0"; continue; }
        let best = 0, bestD = 1e12;
        for (let i = 0; i < state.colorCount; i++) {
          const col = hexToRgb(state.colors[i]);
          const d = (col.r - px[o]) ** 2 + (col.g - px[o + 1]) ** 2 + (col.b - px[o + 2]) ** 2;
          if (d < bestD) { bestD = d; best = i; }
        }
        row += String(best);
      }
      rows.push(row);
    }
    state.rows = rows;
    draw();
    setStatus("PNG разобран в сетку " + state.width + "×" + state.height);
    URL.revokeObjectURL(img.src);
  };
  img.src = URL.createObjectURL(file);
});
function refreshList() {
  fetch("/api/patterns").then(function (r) { return r.json(); }).then(function (names) {
    const box = $("list");
    box.innerHTML = "";
    if (!names.length) { box.textContent = "пока пусто"; return; }
    names.forEach(function (name) {
      const load = document.createElement("button");
      load.type = "button";
      load.textContent = name;
      load.addEventListener("click", function () { loadPattern(name); });
      const del = document.createElement("button");
      del.type = "button";
      del.textContent = "удалить " + name;
      del.addEventListener("click", function () { deletePattern(name); });
      box.appendChild(load);
      box.appendChild(del);
    });
  }).catch(function () { setStatus("нет связи со списком"); });
}
function loadPattern(name) {
  fetch("/api/pattern?id=" + encodeURIComponent(name)).then(function (r) {
    if (!r.ok) throw new Error("не найден");
    return r.json();
  }).then(function (data) {
    if (!data.rows || !data.width || !data.height) throw new Error("в файле нет сетки");
    state.version = 1;
    state.id = data.id || name;
    state.name = data.name || "";
    state.mode = data.mode === undefined ? null : data.mode;
    state.colors = data.colors && data.colors.length === 4 ? data.colors : state.colors;
    state.colorCount = data.colorCount || 2;
    state.width = data.width;
    state.height = data.height;
    state.rows = data.rows;
    state.block = data.block || { number: 1, block: "", holder: "", ranges: "", stops: "" };
    writeForm();
    setStatus("загружен " + name);
  }).catch(function (err) { setStatus(String(err.message || err)); });
}
function deletePattern(name) {
  fetch("/api/pattern?id=" + encodeURIComponent(name), { method: "DELETE" }).then(function (r) {
    if (!r.ok) throw new Error("не удалён");
    setStatus("удалён " + name);
    refreshList();
  }).catch(function (err) { setStatus(String(err.message || err)); });
}
writeForm();
refreshList();
</script>
</body>
</html>
)rawliteral";
