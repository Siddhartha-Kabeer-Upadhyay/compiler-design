
/* Glint Extended Pixel Editor */
(function(){
const $ = s => document.querySelector(s);
const canvas = $('#pixel-canvas');
const ctx = canvas.getContext('2d');
let W=8, H=8, zoom=1, pixels=[], tool='pencil', selColorIdx=0;
let undoStack=[], redoStack=[], painting=false, lastPx=null;

// --- INIT PALETTE ---
const palette = $('#color-palette');
const {colors, OPS} = CONSTANTS;
colors.forEach((c,i)=>{
  const el = document.createElement('div');
  el.className = 'color-swatch' + (c.pixelType==='HALT'?' swatch-black':'') + (c.a===0?' swatch-transparent':'');
  el.style.background = c.a===0 ? '' : `rgb(${c.r},${c.g},${c.b})`;
  el.title = c.name;
  el.dataset.idx = i;
  if(i===0) el.classList.add('active');
  el.addEventListener('click', ()=> selectColor(i));
  palette.appendChild(el);
});

function selectColor(i){
  selColorIdx = i;
  palette.querySelectorAll('.color-swatch').forEach((e,j)=> e.classList.toggle('active', j===i));
  const c = colors[i];
  $('#color-swatch').style.background = c.a===0?'transparent':`rgb(${c.r},${c.g},${c.b})`;
  $('#color-name').textContent = c.name;
  $('#info-rgb').textContent = `${c.r}, ${c.g}, ${c.b}, ${c.a}`;
  $('#info-hsv').textContent = `${c.hsv.h}°, ${c.hsv.s}%, ${c.hsv.v}`;
  $('#info-type').textContent = c.pixelType;
  $('#info-bank').textContent = c.bank;
  const op = OPS[c.instruction] || {};
  $('#info-instr').textContent = c.instruction;
  $('#info-operation').textContent = op.desc||'—';
  $('#info-stack').textContent = op.stack||'—';
}
selectColor(0);

// --- CUSTOM COLORS ---
function addAndSelectCustomColor(r, g, b, namePrefix) {
  const hsv = CONSTANTS.rgbToHsv(r, g, b);
  const p = {r, g, b, a: 255};
  const instr = CONSTANTS.decodeInstrFromPixel(p, hsv);
  const bank = hsv.s > 60 ? 'Extended' : (hsv.s > 20 ? 'Base' : 'Data');
  const type = (r===0 && g===0 && b===0) ? 'HALT' : (hsv.s <= 20 ? 'DATA' : 'CODE');
  
  const c = {
    name: `${namePrefix} → ${instr}`,
    r, g, b, a: 255,
    instruction: type==='DATA' ? 'DATA' : instr,
    bank,
    hsv,
    pixelType: type,
    dataValue: type==='DATA' ? Math.max(r,g,b) : undefined
  };
  
  colors.push(c);
  const i = colors.length - 1;
  const el = document.createElement('div');
  el.className = 'color-swatch' + (type==='HALT'?' swatch-black':'');
  el.style.background = `rgb(${r},${g},${b})`;
  el.title = c.name;
  el.dataset.idx = i;
  el.addEventListener('click', ()=> selectColor(i));
  palette.appendChild(el);
  selectColor(i);
}

$('#btn-add-data').addEventListener('click', () => {
  const val = Math.max(0, Math.min(255, parseInt($('#data-input').value) || 0));
  const rgb = CONSTANTS.hsvToRgb(0, 10, val);
  addAndSelectCustomColor(rgb[0], rgb[1], rgb[2], `Data (${val})`);
});

$('#native-picker').addEventListener('change', (e) => {
  const hex = e.target.value;
  const r = parseInt(hex.slice(1,3), 16);
  const g = parseInt(hex.slice(3,5), 16);
  const b = parseInt(hex.slice(5,7), 16);
  addAndSelectCustomColor(r, g, b, `Custom HEX`);
});

// --- PIXEL GRID ---
function initPixels(){
  pixels = [];
  for(let y=0;y<H;y++){
    pixels[y]=[];
    for(let x=0;x<W;x++) pixels[y][x]={r:0,g:0,b:0,a:0};
  }
}
function updateDims(){
  $('#dim-width').value=W; $('#dim-height').value=H;
  fitZoom(); render();
}

function resize(w,h,keep){
  const old=pixels;
  W=Math.min(Math.max(1,w),1024);
  H=Math.min(Math.max(1,h),1024);
  initPixels();
  if(keep&&old){
    for(let y=0;y<Math.min(H,old.length);y++)
      for(let x=0;x<Math.min(W,(old[y]||[]).length);x++)
        pixels[y][x]=old[y][x];
  }
  updateDims();
}

function fitZoom(){
  const area=$('#canvas-container');
  const maxW=area.clientWidth-40, maxH=area.clientHeight-40;
  zoom=Math.max(1,Math.min(Math.floor(maxW/W),Math.floor(maxH/H)));
  if(W>128||H>128) zoom=Math.max(1,Math.min(Math.floor(maxW/W),Math.floor(maxH/H)));
  updateStatus();
}

function render(){
  canvas.width=W*zoom; canvas.height=H*zoom;
  ctx.clearRect(0,0,canvas.width,canvas.height);
  for(let y=0;y<H;y++) for(let x=0;x<W;x++){
    const p=pixels[y][x];
    if(p.a===0){
      ctx.fillStyle='#999'; ctx.fillRect(x*zoom,y*zoom,zoom,zoom);
      ctx.fillStyle='#666';
      ctx.fillRect(x*zoom,y*zoom,zoom/2,zoom/2);
      ctx.fillRect(x*zoom+zoom/2,y*zoom+zoom/2,zoom/2,zoom/2);
    } else {
      ctx.fillStyle=`rgba(${p.r},${p.g},${p.b},${p.a/255})`;
      ctx.fillRect(x*zoom,y*zoom,zoom,zoom);
    }
  }
  // grid lines
  if(zoom>=4){
    ctx.strokeStyle='rgba(255,255,255,0.08)';
    ctx.lineWidth=0.5;
    for(let x=0;x<=W;x++){ctx.beginPath();ctx.moveTo(x*zoom,0);ctx.lineTo(x*zoom,H*zoom);ctx.stroke();}
    for(let y=0;y<=H;y++){ctx.beginPath();ctx.moveTo(0,y*zoom);ctx.lineTo(W*zoom,y*zoom);ctx.stroke();}
  }
  updateStatus();
}

function updateStatus(){
  $('#status-dim').textContent=`Canvas: ${W} × ${H}`;
  $('#status-zoom').textContent=`Zoom: ${zoom}x`;
}

function getPixelPos(e){
  const r=canvas.getBoundingClientRect();
  return [Math.floor((e.clientX-r.left)/zoom), Math.floor((e.clientY-r.top)/zoom)];
}

// --- UNDO ---
function saveUndo(){
  undoStack.push(JSON.parse(JSON.stringify({w:W,h:H,px:pixels})));
  if(undoStack.length>50) undoStack.shift();
  redoStack=[];
}
function doUndoRedo(from, to){
  if(!from.length) return;
  to.push(JSON.parse(JSON.stringify({w:W,h:H,px:pixels})));
  const s=from.pop(); W=s.w; H=s.h; pixels=s.px;
  updateDims();
}
function undo(){ doUndoRedo(undoStack, redoStack); }
function redo(){ doUndoRedo(redoStack, undoStack); }

// --- TOOLS ---
function applyTool(x,y){
  if(x<0||x>=W||y<0||y>=H) return;
  const c=colors[selColorIdx];
  if(tool==='pencil') pixels[y][x]={r:c.r,g:c.g,b:c.b,a:c.a};
  else if(tool==='eraser') pixels[y][x]={r:0,g:0,b:0,a:0};
  else if(tool==='fill') floodFill(x,y,c);
  else if(tool==='eyedropper'){
    const p=pixels[y][x];
    const idx=colors.findIndex(c=>c.r===p.r&&c.g===p.g&&c.b===p.b&&c.a===p.a);
    if(idx>=0) selectColor(idx);
  }
  render();
}

function floodFill(sx,sy,c){
  const t=pixels[sy][sx], tr=t.r,tg=t.g,tb=t.b,ta=t.a;
  if(tr===c.r&&tg===c.g&&tb===c.b&&ta===c.a) return;
  const q=[[sx,sy]]; const seen=new Set();
  while(q.length){
    const [x,y]=q.pop(); const k=y*W+x;
    if(seen.has(k)||x<0||x>=W||y<0||y>=H) continue;
    const p=pixels[y][x];
    if(p.r!==tr||p.g!==tg||p.b!==tb||p.a!==ta) continue;
    seen.add(k);
    pixels[y][x]={r:c.r,g:c.g,b:c.b,a:c.a};
    q.push([x+1,y],[x-1,y],[x,y+1],[x,y-1]);
  }
}

// --- CANVAS EVENTS ---
canvas.addEventListener('mousedown', e=>{
  painting=true; saveUndo();
  const [x,y]=getPixelPos(e); lastPx=`${x},${y}`;
  applyTool(x,y);
});
canvas.addEventListener('mousemove', e=>{
  const [x,y]=getPixelPos(e);
  $('#status-pos').textContent=`Pos: ${x}, ${y}`;
  if(x>=0&&x<W&&y>=0&&y<H){
    const p=pixels[y][x];
    $('#status-color').textContent=`Color: ${p.a===0 ? 'Transparent' : `rgb(${p.r},${p.g},${p.b})`}`;
    const hsv=CONSTANTS.rgbToHsv(p.r,p.g,p.b);
    const instrName = CONSTANTS.decodeInstrFromPixel(p, hsv);
    $('#status-instr').textContent=`Instruction: ${instrName}`;
  }
  if(!painting) return;
  const k=`${x},${y}`;
  if(k===lastPx) return; lastPx=k;
  applyTool(x,y);
});
canvas.addEventListener('mouseup', ()=>{painting=false;lastPx=null;});
canvas.addEventListener('mouseleave', ()=>{painting=false;lastPx=null;});

// --- TOOLBAR ---
document.querySelectorAll('.tool-btn[data-tool]').forEach(b=>{
  b.addEventListener('click',()=>{
    document.querySelectorAll('.tool-btn[data-tool]').forEach(t=>t.classList.remove('active'));
    b.classList.add('active');
    tool=b.dataset.tool;
  });
});

$('#btn-zoom-in').addEventListener('click',()=>{zoom=Math.min(zoom+1,64);render();});
$('#btn-zoom-out').addEventListener('click',()=>{zoom=Math.max(zoom-1,1);render();});
$('#btn-zoom-fit').addEventListener('click',()=>{fitZoom();render();});
$('#btn-undo').addEventListener('click', undo);
$('#btn-redo').addEventListener('click', redo);
$('#btn-clear').addEventListener('click',()=>{saveUndo();initPixels();render();});

// --- DIMENSIONS ---
$('#btn-resize').addEventListener('click',()=>{
  saveUndo();
  resize(+$('#dim-width').value, +$('#dim-height').value, true);
});
document.querySelectorAll('.preset-btn').forEach(b=>{
  b.addEventListener('click',()=>{
    saveUndo();
    resize(+b.dataset.w, +b.dataset.h, true);
  });
});

// --- DOWNLOAD ---
function getBase64(){
  const c2=document.createElement('canvas');
  c2.width=W; c2.height=H;
  const x2=c2.getContext('2d'), id=x2.createImageData(W,H);
  for(let y=0;y<H;y++) for(let x=0;x<W;x++){
    const p=pixels[y][x], i=(y*W+x)*4;
    id.data[i]=p.r; id.data[i+1]=p.g; id.data[i+2]=p.b; id.data[i+3]=p.a;
  }
  x2.putImageData(id,0,0);
  return c2.toDataURL('image/png');
}

$('#btn-download').addEventListener('click',()=>{
  const a=document.createElement('a');
  a.download='glint_program.png';
  a.href=getBase64();
  a.click();
});

// --- OPEN / DRAG-DROP ---
$('#btn-open').addEventListener('click',()=>$('#file-input').click());
$('#file-input').addEventListener('change', e=>{if(e.target.files[0]) loadImage(e.target.files[0]);});

document.body.addEventListener('dragover', e=>{e.preventDefault();$('#drop-zone').classList.remove('hidden');});
document.body.addEventListener('dragleave', e=>{
  if(!e.relatedTarget || !document.body.contains(e.relatedTarget)) $('#drop-zone').classList.add('hidden');
});
document.body.addEventListener('drop', e=>{
  e.preventDefault();$('#drop-zone').classList.add('hidden');
  if(e.dataTransfer.files[0]) loadImage(e.dataTransfer.files[0]);
});

function loadImage(file){
  const reader=new FileReader();
  reader.onload=function(e){
    const img=new Image();
    img.onload=function(){
      if(img.width>1024||img.height>1024){
        log('ERROR: Image exceeds 1024×1024 max','error');return;
      }
      saveUndo();
      W=img.width; H=img.height;
      const tc=document.createElement('canvas');
      tc.width=W; tc.height=H;
      const tx=tc.getContext('2d');
      tx.drawImage(img,0,0);
      const d=tx.getImageData(0,0,W,H).data;
      pixels=[];
      for(let y=0;y<H;y++){
        pixels[y]=[];
        for(let x=0;x<W;x++){
          const i=(y*W+x)*4;
          pixels[y][x]={r:d[i],g:d[i+1],b:d[i+2],a:d[i+3]};
        }
      }
      updateDims();
      log(`Loaded image: ${W}×${H}`,'success');
    };
    img.src=e.target.result;
  };
  reader.readAsDataURL(file);
}

// --- NEW ---
$('#btn-new').addEventListener('click',()=>{saveUndo();resize(8,8,false);log('New 8×8 canvas','info');});

// --- RUN (client-side interpreter) ---
$('#btn-run').addEventListener('click',()=>$('#run-modal').classList.remove('hidden'));
$('#modal-close').addEventListener('click',()=>$('#run-modal').classList.add('hidden'));
$('#modal-cancel').addEventListener('click',()=>$('#run-modal').classList.add('hidden'));
$('.modal-backdrop').addEventListener('click',()=>$('#run-modal').classList.add('hidden'));
$('#run-mode').addEventListener('change', e=>{
  $('#trace-limit-field').style.display = e.target.value==='trace'?'':'none';
});

$('#modal-run').addEventListener('click',()=>{
  $('#run-modal').classList.add('hidden');
  const mode=$('#run-mode').value;
  const stdin=$('#run-stdin').value;
  const limit=+$('#trace-limit').value||1000;

  runBackend(mode, limit, stdin);
});

function runBackend(mode, limit, stdin) {
  log(`── Running on C Backend (WSL) ──`, 'info');
  fetch('/api/run', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ image: getBase64(), mode, limit, stdin })
  })
  .then(res => res.json())
  .then(data => {
      if (data.error) log(`Server Error: ${data.error}`, 'error');
      else log(data.output);
  })
  .catch(err => log(`Fetch Error: ${err.message}`, 'error'));
}

function log(msg, cls){
  const con=$('#output-console');
  const span=document.createElement('span');
  if(cls) span.className='output-'+cls;
  span.textContent=msg+'\n';
  con.appendChild(span);
  con.scrollTop=con.scrollHeight;
}
$('#btn-clear-output').addEventListener('click',()=>{$('#output-console').innerHTML='';});


// --- KEYBOARD ---
document.addEventListener('keydown', e=>{
  if(e.ctrlKey&&e.key==='z'){e.preventDefault();undo();}
  if(e.ctrlKey&&e.key==='y'){e.preventDefault();redo();}
  if(!e.ctrlKey){
    if(e.key==='p') document.querySelector('[data-tool="pencil"]').click();
    if(e.key==='e') document.querySelector('[data-tool="eraser"]').click();
    if(e.key==='f') document.querySelector('[data-tool="fill"]').click();
    if(e.key==='i') document.querySelector('[data-tool="eyedropper"]').click();
    if(e.key==='='||e.key==='+') $('#btn-zoom-in').click();
    if(e.key==='-') $('#btn-zoom-out').click();
  }
});

// --- RESIZABLE TERMINAL ---
const resizer = $('#output-resizer');
const outputPanel = $('#output-panel');
let isResizing = false;

resizer.addEventListener('mousedown', (e) => {
  isResizing = true;
  document.body.style.cursor = 'ns-resize';
});

document.addEventListener('mousemove', (e) => {
  if (!isResizing) return;
  // Calculate new height based on mouse position
  // The height should be window height - mouse Y position
  let newHeight = window.innerHeight - e.clientY;
  
  // Clamp height between 80px (min) and 80% of window height (max)
  if (newHeight > 80 && newHeight < window.innerHeight * 0.8) {
    outputPanel.style.height = `${newHeight}px`;
  }
});

document.addEventListener('mouseup', () => {
  if (isResizing) {
    isResizing = false;
    document.body.style.cursor = '';
    fitZoom();
    render();
  }
});

// --- INIT ---
initPixels(); fitZoom(); render();
window.addEventListener('resize',()=>{fitZoom();render();});
})();
