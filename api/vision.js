const GOAL_PROMPT = `This image shows ONLY the inside of a Subbuteo miniature football goal net. The image has been cropped to show nothing but the net interior.

Your only job: is there a small round ball visible anywhere in this image?

- The ball is small, round, and will appear somewhere inside the net area
- If you can see the ball anywhere in this image, it is a goal
- ballSeen: true = GOAL. ballSeen: false = no goal.

Respond ONLY with valid JSON, no other text:
{"ballSeen": true or false, "confidence": "high" or "medium" or "low", "observation": "one short sentence"}`;

module.exports = async function handler(req, res) {
  if (req.method !== 'POST') return res.status(405).end();

  const { image } = req.body || {};
  if (!image) return res.status(400).json({ error: 'No image provided' });

  const apiKey = process.env.Anthropic_API_Key;
  if (!apiKey) return res.status(500).json({ error: 'Anthropic_API_Key env var not set' });

  const upstream = await fetch('https://api.anthropic.com/v1/messages', {
    method: 'POST',
    headers: {
      'x-api-key': apiKey,
      'anthropic-version': '2023-06-01',
      'content-type': 'application/json',
    },
    body: JSON.stringify({
      model: 'claude-haiku-4-5-20251001',
      max_tokens: 60,
      messages: [{
        role: 'user',
        content: [
          { type: 'image', source: { type: 'base64', media_type: 'image/jpeg', data: image } },
          { type: 'text', text: GOAL_PROMPT }
        ]
      }]
    })
  });

  const data = await upstream.json();
  res.status(upstream.status).json(data);
};
