(function(window) {
    "use strict";
    const Cero = window.Cero = window.Cero || {};
    const state = Cero.state = Cero.state || {};
let selectedVersion = null;
let skinViewer = null;
let allVersions = [];
let currentFilter = 'release';

function setFilter(f) {
    currentFilter = f; state.currentFilter = currentFilter;
    document.querySelectorAll('.dd-tab').forEach(b => b.classList.remove('active'));
    const btn = document.getElementById('tab-' + f);
    if (btn) btn.classList.add('active');
    renderDropdown();
}

function updatePlayButton() {
    const btn = document.getElementById('launchBtn');
    if (!selectedVersion) return;
    btn.innerHTML = `▶ Jouer ${selectedVersion.id}`;
    btn.disabled = false;
    btn.style.opacity = '';
}

function persistSelection() {
    if (!selectedVersion || !window.setVersion) return;
    window.setVersion(selectedVersion.id);
}

async function loadVersions() {
    try {
        const r = await fetch('https://launchermeta.mojang.com/mc/game/version_manifest.json');
        const data = await r.json();
        allVersions = data.versions;

        const saved = await window.getVersion();

        selectedVersion =
            (saved && allVersions.find(v => v.id === saved)) ||
            allVersions.find(v => v.id === data.latest.release) ||
            allVersions[0];

        document.getElementById('selectedVersionDisplay').textContent = selectedVersion.id;
        state.selectedVersion = selectedVersion;
        renderDropdown();
        updatePlayButton();
    } catch (e) { console.error('loadVersions:', e); }
}

function selectVersion(id) {
    const v = allVersions.find(x => x.id === id);
    if (!v) return;
    selectedVersion = v; state.selectedVersion = selectedVersion;
    document.getElementById('selectedVersionDisplay').textContent = v.id;
    persistSelection();
    updatePlayButton();
    closeVersionDropdown();
}

function renderDropdown() {
    const q = (document.getElementById('ddSearch').value || '').toLowerCase();
    const list = allVersions.filter(v => {
        if (currentFilter !== 'all' && v.type !== currentFilter) return false;
        if (q && !v.id.toLowerCase().includes(q)) return false;
        return true;
    }).slice(0, 200);

    const ddList = document.getElementById('ddList');
    ddList.innerHTML = list.map(v => {
        return `
            <div class="dd-item" onclick="selectVersion('${v.id}')">
                <span>${v.id}</span>
                <span class="dd-type">${v.type}</span>
            </div>
        `;
    }).join('');
}

function toggleVersionDropdown(e) {
    e.stopPropagation();
    const dd = document.getElementById('versionDropdown');
    if (dd.classList.contains('open')) { closeVersionDropdown(); return; }

    dd.style.visibility = 'hidden';
    dd.classList.add('open');
    renderDropdown();

    requestAnimationFrame(() => {
        const grp = document.getElementById('playGroup');
        const dd = document.getElementById('versionDropdown');
        const rGrp = grp.getBoundingClientRect();
        
        if (dd.parentElement !== document.body) {
            document.body.appendChild(dd);
        }
        
        dd.style.position = 'fixed'; 
        const left = Math.max(8, rGrp.right - dd.offsetWidth);
        dd.style.left = left + 'px';
        
        const maxH = Math.min(300, rGrp.top - 16); 
        dd.style.maxHeight = maxH + 'px';
        dd.style.bottom = (window.innerHeight - rGrp.top + 4) + 'px';
        
        dd.style.top = 'auto';
        dd.style.right = 'auto';
        dd.style.visibility = 'visible';

        document.getElementById('ddSearch').focus();
    });
}

function closeVersionDropdown() {
    const dd = document.getElementById('versionDropdown');
    if (dd) dd.classList.remove('open');
}

document.addEventListener('click', (e) => {
    const dd = document.getElementById('versionDropdown');
    if (!dd || !dd.classList.contains('open')) return;
    if (e.target.closest('#versionDropdown') || e.target.closest('#playArrowBtn')) return;
    closeVersionDropdown();
});

function initSkinViewer(skinUrl) {
    const canvasEl = document.getElementById('skinViewer');
    const parent   = document.getElementById('skinContainer');

    function getSize() {
        const r = parent.getBoundingClientRect();
        return { w: Math.max(200, Math.floor(r.width)), h: Math.max(200, Math.floor(r.height)) };
    }

    if (skinViewer) { skinViewer.loadSkin(skinUrl).catch(() => {}); return; }
    if (typeof skinview3d === 'undefined') return;

    requestAnimationFrame(() => {
        const { w, h } = getSize();
        try {
            skinViewer = new skinview3d.SkinViewer({ canvas: canvasEl, width: w, height: h });
            skinViewer.loadSkin(skinUrl).catch(() => skinViewer.loadSkin('https://mc-heads.net/skin/MHF_Steve'));
            skinViewer.fov = 35;
            skinViewer.zoom = 0.85;
            skinViewer.controls.enableZoom = false;
            skinViewer.controls.enablePan  = false;
            skinViewer.animation = new skinview3d.WalkingAnimation();
            skinViewer.animation.speed = 0.5;
            new ResizeObserver(() => {
                const s = getSize();
                skinViewer.setSize(s.w, s.h);
            }).observe(parent);
        } catch(e) { console.error('SkinViewer:', e); }
    });
}

async function loadAccount() {
    try {
        const raw = await window.getAccount();
        const acc  = typeof raw === 'string' ? JSON.parse(raw) : raw;
        const name = acc.name || acc.username || 'Joueur';
        const uuid = acc.uuid || acc.id || '';

        document.getElementById('playerName').textContent = name;
    
        if (uuid) {
            window.myUuid = uuid;
            initSkinViewer(`https://mc-heads.net/skin/${uuid}`);
            const av = document.getElementById('overlayAvatar');
            av.src = `https://mc-heads.net/avatar/${uuid}/64`;
            av.classList.remove('hidden');
            document.getElementById('overlayIcon').classList.add('hidden');
        }
    } catch(e) { console.error('loadAccount:', e); }
}

    Cero.modules = Cero.modules || {};
    Cero.modules.setFilter = setFilter; window.setFilter = setFilter;
    Cero.modules.updatePlayButton = updatePlayButton; window.updatePlayButton = updatePlayButton;
    Cero.modules.persistSelection = persistSelection; window.persistSelection = persistSelection;
    Cero.modules.loadVersions = loadVersions; window.loadVersions = loadVersions;
    Cero.modules.selectVersion = selectVersion; window.selectVersion = selectVersion;
    Cero.modules.renderDropdown = renderDropdown; window.renderDropdown = renderDropdown;
    Cero.modules.toggleVersionDropdown = toggleVersionDropdown; window.toggleVersionDropdown = toggleVersionDropdown;
    Cero.modules.closeVersionDropdown = closeVersionDropdown; window.closeVersionDropdown = closeVersionDropdown;
    Cero.modules.initSkinViewer = initSkinViewer; window.initSkinViewer = initSkinViewer;
    Cero.modules.loadAccount = loadAccount; window.loadAccount = loadAccount;
})(window);
