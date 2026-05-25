const VISION_PROMPT = `You are analysing a photo of a Subbuteo tabletop football game.

THE BALL: a small sphere, about 1 cm diameter, white or off-white, sits on the green felt and casts a small round shadow. It is THREE-DIMENSIONAL. It is SMALLER than any player figure base. There is only ONE ball.

PLAYER BASES: large flat circular discs pressed against the felt, 2-3 cm diameter. Much bigger than the ball. Do not confuse these with the ball.

HUMAN HANDS: may be visible in the frame. Ignore them completely.

Answer these two questions:
1. Can you see the ball anywhere in the image?
2. If yes — is it clearly inside a goal net? (The goal net is a rectangular frame at either short end of the pitch. The ball must be past the goal line, inside the net structure, not just in front of the posts.)

Respond ONLY with valid JSON, no other text:
{
  "ballSeen": true or false,
  "inGoal": true or false,
  "confidence": "high" or "medium" or "low",
  "observation": "One sentence describing exactly what you see, e.g. Ball visible near centre circle or Ball inside left goal net or No ball visible"
}`;

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
      max_tokens: 120,
      messages: [{
        role: 'user',
        content: [
          { type: 'image', source: { type: 'base64', media_type: 'image/jpeg', data: image } },
          { type: 'text',  text: VISION_PROMPT }
        ]
      }]
    })
  });

  const data = await upstream.json();
  res.status(upstream.status).json(data);
};
