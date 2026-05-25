const GOAL_CROP_PROMPT = `You are looking at a cropped image showing ONE GOAL END of a Subbuteo tabletop football pitch.

THE BALL is a soccer ball — very round, slightly smaller than the player figures on the pitch. It sits on the green felt surface. There is only ONE ball on the entire pitch.

PLAYER FIGURES stand on large flat circular bases. The ball is rounder and slightly smaller than these bases. Do not confuse a player base with the ball.

HUMAN HANDS may be visible in the frame — ignore them completely.

YOUR QUESTION: Is the soccer ball currently inside the goal net shown in this image?

Rules:
- The ball must be PAST the goal line, inside the net structure — not in front of the posts
- A ball sitting in front of or beside the goal does NOT count as a goal
- If you are not confident, say inGoal: false — do not guess
- ballSeen means you can see the ball anywhere in this cropped image

Respond ONLY with valid JSON, no other text:
{"inGoal": true or false, "ballSeen": true or false, "confidence": "high" or "medium" or "low", "observation": "one sentence describing exactly what you see"}`;

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
          { type: 'text',  text: GOAL_CROP_PROMPT }
        ]
      }]
    })
  });

  const data = await upstream.json();
  res.status(upstream.status).json(data);
};
