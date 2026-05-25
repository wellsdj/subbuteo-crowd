const GOAL_CROP_PROMPT = `You are looking at a cropped image of ONE GOAL END of a Subbuteo tabletop football pitch. The camera is positioned at the SIDE of the pitch, so you are seeing the goal from the side or a slight angle — you may not see directly into the net.

THE GOAL is a small rectangular frame (posts + crossbar) with a white net attached behind it, sitting on the green felt surface.

THE BALL is a small, very round soccer ball — slightly smaller than the player figure bases. It sits on the green felt. There is only ONE ball on the whole pitch.

PLAYER FIGURES have large flat circular bases. Do not mistake a player base for the ball.

HANDS or arms may be visible — ignore them.

YOUR QUESTION: Is the soccer ball at or inside the goal — meaning at the goal line or behind the posts?

Rules for inGoal = true:
- Ball is touching or overlapping the goal posts/frame
- Ball is behind the goal line (even partially hidden by the net)
- Ball is inside the net area, even if you can only see part of it
- The camera angle may make it hard to see "inside" the net — if the ball is clearly at the goal structure, say inGoal: true

Rules for inGoal = false:
- Ball is clearly in open play on the pitch, away from the goal
- No ball is visible near the goal at all

ballSeen = true if you can see the ball anywhere in this cropped image.

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
