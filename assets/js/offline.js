const canvas = document.getElementById('grainCanvas');
const ctx = canvas.getContext('2d');
let width, height;

function resize() {
    width  = canvas.width  = window.innerWidth;
    height = canvas.height = window.innerHeight;
}
window.addEventListener('resize', resize);
resize();

function animateGrain() {
    const imageData = ctx.createImageData(width, height);
    const data = imageData.data;
    for (let i = 0; i < data.length; i += 4) {
        const val = Math.random() * 255;
        data[i] = data[i+1] = data[i+2] = val;
        data[i+3] = 255;
    }
    ctx.putImageData(imageData, 0, 0);
    requestAnimationFrame(animateGrain);
}
animateGrain();

document.body.addEventListener('mousedown', (e) => {
    if (e.button !== 0) return;
    if (e.target.closest('a, button, input, select, textarea')) return;
    if (window.drag_start) window.drag_start();
});

const statusText = document.getElementById('statusText');

async function checkAndRedirect() {
    try {
        if (statusText) statusText.textContent = 'Vérification en cours';
        const hasInternet = await window.checkInternet();
        
        if (hasInternet) {
            if (statusText) statusText.textContent = 'Connexion détectée !';
            await new Promise(r => setTimeout(r, 600));
            window.location.href = 'index.html';
        }
    } catch (_) { }
}

checkAndRedirect();

setInterval(checkAndRedirect, 3000);