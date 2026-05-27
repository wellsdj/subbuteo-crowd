const GOAL_PROMPT = `This image shows ONLY the inside of a Subbuteo miniature football goal net, cropped tightly to the net interior.

IMPORTANT — there will almost always be a GOALKEEPER figure visible in this image. The goalkeeper is a small plastic figure on a round flat base, attached to a long thin rod or pole that slides across the goal. This is normal. The goalkeeper is NOT the ball.

Your only job: is there a small round BALL visible in the net, separate from the goalkeeper figure?

The BALL is:
- Small and round, like a tiny soccer ball
- Separate from the goalkeeper and its base
- Sits on the felt surface or rests in the net

ballSeen = true ONLY if you can see the ball itself, not just the goalkeeper figure or its base.

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
