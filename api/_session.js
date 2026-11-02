// Signed session cookie shared by /api/auth and the proxies.
// Files starting with "_" in /api are not exposed as routes by Vercel.
const crypto = require('crypto');

const COOKIE = 'crowd_session';
const MAX_AGE_S = 60 * 60 * 24 * 30; // 30 days

function secret() {
  return process.env.SESSION_SECRET || process.env.CROWD_PASSWORD || '';
}

function sign(payload) {
  return crypto.createHmac('sha256', secret()).update(payload).digest('base64url');
}

function issue(res) {
  const exp = Math.floor(Date.now() / 1000) + MAX_AGE_S;
  const payload = 'v1.' + exp;
  res.setHeader('Set-Cookie',
    `${COOKIE}=${payload}.${sign(payload)}; Path=/; Max-Age=${MAX_AGE_S}; HttpOnly; Secure; SameSite=Strict`);
}

function clear(res) {
  res.setHeader('Set-Cookie', `${COOKIE}=; Path=/; Max-Age=0; HttpOnly; Secure; SameSite=Strict`);
}

function isAuthed(req) {
  if (!secret()) return false;
  const raw = (req.headers.cookie || '').split(';').map(c => c.trim()).find(c => c.startsWith(COOKIE + '='));
  if (!raw) return false;
  const [v, exp, sig] = raw.slice(COOKIE.length + 1).split('.');
  if (v !== 'v1' || !exp || !sig || Number(exp) < Date.now() / 1000) return false;
  return safeEqual(sig, sign(v + '.' + exp));
}

function safeEqual(a, b) {
  const x = Buffer.from(String(a)), y = Buffer.from(String(b));
  return x.length === y.length && crypto.timingSafeEqual(x, y);
}

module.exports = { issue, clear, isAuthed, safeEqual };
