function showCloseMenu(e) {
    e.stopPropagation();
    const m = document.getElementById('closeMenu');
    m.style.display = 'block';
    requestAnimationFrame(() => {
        const r = m.getBoundingClientRect();
        m.style.left = Math.min(e.clientX, innerWidth  - r.width  - 8) + 'px';
        m.style.top  = Math.min(e.clientY, innerHeight - r.height - 8) + 'px';
    });
}

function hideCloseMenu() {
    document.getElementById('closeMenu').style.display = 'none';
}

document.addEventListener('click', (e) => {
    if (!e.target.closest('#closeMenu') && !e.target.closest('.wc-btn.close'))
        hideCloseMenu();
});
