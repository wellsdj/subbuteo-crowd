const GOAL_PROMPT = `The camera is positioned right up against a Subbuteo miniature football goal. The goal fills most of the frame. There may or may not be a small round ball inside the goal net.

Is the ball inside the goal net right now?

- YES (inGoal: true) only if the ball is clearly sitting inside the net, behind the goal line
- NO (inGoal: false) if the ball is not visible, is in front of the goal, or is beside the goal
- Do not say inGoal: true unless you are confident the ball is inside the net

Respond ONLY with valid JSON, no other text:
{"inGoal": true or false, "confidence": "high" or "medium" or "low", "observation": "one short sentence"}`;

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
      max_tokens: 80,
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
