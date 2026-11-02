const { isAuthed } = require('./_session');

// The model and token cap are fixed here, not taken from the browser, so the
// endpoint can't be used to run arbitrary requests on this account.
const MODEL = 'claude-sonnet-5-5';
const MAX_TOKENS_CAP = 4000;

module.exports = async function handler(req, res) {
  if (req.method !== 'POST') return res.status(405).end();
  if (!isAuthed(req)) return res.status(401).json({ error: { message: 'Session expired — reload and enter the access code' } });

  const { max_tokens, messages, system } = req.body || {};
  const apiKey = process.env.ANTHROPIC_API_KEY;
  if (!apiKey) return res.status(500).json({ error: { message: 'ANTHROPIC_API_KEY not configured' } });
  if (!Array.isArray(messages) || !messages.length) return res.status(400).json({ error: { message: 'messages required' } });

  try {
    const body = {
      model: MODEL,
      // Thinking tokens count toward max_tokens, so leave headroom above what the client asked for.
      max_tokens: Math.min(MAX_TOKENS_CAP, (Number(max_tokens) || 1000) + 1500),
      messages,
      output_config: { effort: 'low' }, // short scripted lines; keep latency down
      fallbacks: 'default',
    };
    if (typeof system === 'string') body.system = system;

    const upstream = await fetch('https://api.anthropic.com/v1/messages', {
      method: 'POST',
      headers: {
        'x-api-key': apiKey,
        'anthropic-version': '2023-06-01',
        'anthropic-beta': 'server-side-fallback-2026-07-01',
        'content-type': 'application/json',
      },
      body: JSON.stringify(body),
    });

    const data = await upstream.json();
    if (!upstream.ok) return res.status(upstream.status).json(data);
    if (data.stop_reason === 'refusal') return res.status(422).json({ error: { message: 'The model declined this request' } });
    // Hand back just the text so the client doesn't have to skip thinking blocks.
    const text = (data.content || []).filter(b => b.type === 'text').map(b => b.text).join('');
    res.status(200).json({ text, content: [{ type: 'text', text }] });
  } catch (e) {
    res.status(500).json({ error: { message: e.message } });
  }
};
