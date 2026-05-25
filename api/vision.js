const GOAL_CROP_PROMPT = `You are looking at a cropped image of ONE GOAL END of a Subbuteo tabletop football pitch.

CAMERA POSITION: The laptop is placed at the LONG SIDE of the pitch, camera roughly at pitch level. The goals are at the LEFT and RIGHT ends of the full frame. This crop shows just one of those ends — the goal appears near the LEFT or RIGHT EDGE of this image, seen from the side.

THE GOAL is a small rectangular frame (two posts + crossbar) with a white net behind it, sitting on the green felt.

THE BALL is a small very round soccer ball, slightly smaller than the player figure bases. There is only ONE ball on the whole pitch.

PLAYER FIGURES have large flat circular bases — do not mistake them for the ball.

HANDS or arms may be visible — ignore them completely.

YOUR QUESTION: Is the soccer ball inside or at the goal in this image?

inGoal = true when:
- The ball is touching or overlapping the goal posts or crossbar
- The ball is behind the goal posts (inside the net), even partially hidden
- The ball is between the posts at the goal line

inGoal = false when:
- The ball is on the open pitch, clearly away from the goal
- No ball is visible near the goal

ballSeen = true if the ball is visible anywhere in this image.

Respond ONLY with valid JSON, no other text:
{"inGoal": true or false, "ballSeen": true or false, "confidence": "high" or "medium" or "low", "observation": "one sentence describing what you see"}`;

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
