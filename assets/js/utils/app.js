document.addEventListener('contextmenu', e => e.preventDefault());

document.addEventListener('selectstart', e => {
  const t = e.target;
  if (t.tagName === 'INPUT' || t.tagName === 'TEXTAREA' || t.isContentEditable) return;
  e.preventDefault();
}, true);

document.addEventListener('dragstart', e => {
  if (e.target.tagName === 'IMG') e.preventDefault();
}, true);

window.addEventListener('keydown', e => {
  const k = e.key.toLowerCase();
  const ctrl = e.ctrlKey || e.metaKey;
  if (ctrl && ['f','p','g','s','u','j','h','d','a'].includes(k)) e.preventDefault();
  if (['F3','F5','F7'].includes(e.key)) e.preventDefault();
  if (ctrl && e.shiftKey && k === 'r') e.preventDefault();
}, true);
