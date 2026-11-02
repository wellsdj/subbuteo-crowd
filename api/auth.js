const { issue, clear, isAuthed, safeEqual } = require('./_session');

module.exports = function handler(req, res) {
  // GET: is this browser already signed in? (lets returning users skip the access code)
  if (req.method === 'GET') return res.status(isAuthed(req) ? 200 : 401).json({ ok: isAuthed(req) });
  if (req.method === 'DELETE') { clear(res); return res.status(200).json({ ok: true }); }
  if (req.method !== 'POST') return res.status(405).end();

  const { password } = req.body || {};
  const expected = process.env.CROWD_PASSWORD;
  if (expected && password && safeEqual(password, expected)) {
    issue(res);
    res.status(200).json({ ok: true });
  } else {
    res.status(401).json({ ok: false });
  }
};
