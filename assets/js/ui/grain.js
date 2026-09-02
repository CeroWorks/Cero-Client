(function () {
    'use strict';

    var TILE = 128;
    var FRAMES = 8;
    var FPS = 24;

    function start() {
        var canvas = document.getElementById('grainCanvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d', { alpha: true });
        if (!ctx) return;

        var tiles = [];
        for (var f = 0; f < FRAMES; f++) {
            var tile = document.createElement('canvas');
            tile.width = tile.height = TILE;
            var tctx = tile.getContext('2d');
            var img = tctx.createImageData(TILE, TILE);
            var d = img.data;
            for (var i = 0; i < d.length; i += 4) {
                var v = (Math.random() * 255) | 0;
                d[i] = d[i + 1] = d[i + 2] = v;
                d[i + 3] = 255;
            }
            tctx.putImageData(img, 0, 0);
            tiles.push(tile);
        }

        var patterns = tiles.map(function (t) { return ctx.createPattern(t, 'repeat'); });

        var width = 0, height = 0;
        function resize() {
            width = canvas.width = window.innerWidth;
            height = canvas.height = window.innerHeight;
            patterns = tiles.map(function (t) { return ctx.createPattern(t, 'repeat'); });
        }
        window.addEventListener('resize', resize);
        resize();

        var frame = 0;
        var last = 0;
        var interval = 1000 / FPS;
        var rafId = null;
        var running = true;

        function animate(now) {
            rafId = window.requestAnimationFrame(animate);
            if (!running) return;
            if (now - last < interval) return;
            last = now;

            frame = (frame + 1) % FRAMES;
            ctx.clearRect(0, 0, width, height);
            var ox = (Math.random() * TILE) | 0;
            var oy = (Math.random() * TILE) | 0;
            ctx.save();
            ctx.translate(-ox, -oy);
            ctx.fillStyle = patterns[frame];
            ctx.fillRect(0, 0, width + TILE, height + TILE);
            ctx.restore();
        }
        rafId = window.requestAnimationFrame(animate);

        document.addEventListener('visibilitychange', function () {
            running = !document.hidden;
        });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', start, { once: true });
    } else {
        start();
    }
})();