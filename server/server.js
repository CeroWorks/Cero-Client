const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const rateLimit = require('express-rate-limit');
const http = require('http');

const friendsRouter = require('./routes/friends');
const messagesRouter = require('./routes/messages');
const setupWebSocket = require('./ws');

const app = express();
app.use(helmet());
app.use(cors({ origin: true }));
app.use(express.json({ limit: '32kb' }));

app.use('/api/', rateLimit({
  windowMs: 60 * 1000,
  max: 120,
  standardHeaders: true,
  legacyHeaders: false
}));

app.use('/api/messages', rateLimit({
  windowMs: 10 * 1000,
  max: 20,
  message: { error: 'rate_limited' }
}));

app.get('/health', (req, res) => res.json({ ok: true, time: Date.now() }));

app.use('/api/friends', friendsRouter);
app.use('/api/messages', messagesRouter);

app.use((err, req, res, next) => {
  console.error(err);
  res.status(500).json({ error: 'internal' });
});

const server = http.createServer(app);
const ws = setupWebSocket(server);
app.set('ws', ws);

const PORT = process.env.PORT || 3134;
server.listen(PORT, () => {
  console.log(`CeroClient server on http://localhost:${PORT}`);
});
