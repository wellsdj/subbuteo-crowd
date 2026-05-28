module.exports = async function handler(req, res) {
  if (req.method !== 'POST') return res.status(405).end();

  const { voice_id, text, model_id, voice_settings } = req.body || {};
  const apiKey = process.env.ELEVEN_API_KEY;
  if (!apiKey) return res.status(500).json({ detail: { message: 'ELEVEN_API_KEY not configured' } });

  try {
    const upstream = await fetch(`https://api.elevenlabs.io/v1/text-to-speech/${voice_id}`, {
      method: 'POST',
      headers: {
        'xi-api-key': apiKey,
        'content-type': 'application/json',
      },
      body: JSON.stringify({ text, model_id, voice_settings }),
    });

    if (!upstream.ok) {
      const err = await upstream.json().catch(() => ({}));
      return res.status(upstream.status).json(err);
    }

    const buffer = await upstream.arrayBuffer();
    res.setHeader('Content-Type', 'audio/mpeg');
    res.status(200).send(Buffer.from(buffer));
  } catch (e) {
    res.status(500).json({ detail: { message: e.message } });
  }
};
