const GOAL_PROMPT = `You are looking at the inside of a Subbuteo miniature football goal. Here is exactly what you will see:

THE GOAL FRAME: White plastic rectangular posts and crossbar. Red/orange string net forming a grid pattern inside.

THE GOALKEEPER: Always present. A small green plastic figure standing upright on a flat circular white disc base. The base and figure are mounted on a long white plastic rod that extends out through the front of the goal. The goalkeeper is always there — ignore it completely.

THE BALL (if present): A large white sphere, smooth and round. Noticeably larger than the goalkeeper's disc base. It will sit on the green felt surface inside the net. It is very clearly a ball — round, white, solid.

YOUR TASK: Is the white ball present inside the net, separate from the goalkeeper figure?

- YES (ballSeen: true) if you can see the large white sphere sitting in the net
- NO (ballSeen: false) if you only see the goalkeeper figure and empty net — no ball

Think carefully. The goalkeeper base is a flat disc on a rod. The ball is a large round sphere sitting on the surface. They look completely different.

Respond ONLY with valid JSON:
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
      model: 'claude-sonnet-4-5-20251001',
      max_tokens: 1200,
      thinking: { type: 'enabled', budget_tokens: 1024 },
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
