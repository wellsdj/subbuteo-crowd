const VISION_PROMPT = `You are checking a single yes/no question about a Subbuteo tabletop football game.

QUESTION: Is the Subbuteo ball currently INSIDE either goal net?

WHAT THE BALL LOOKS LIKE:
- Small sphere, approximately 1 cm diameter
- White or off-white colour
- Sits on top of the felt — it is THREE-DIMENSIONAL, casting a small round shadow
- It is SMALLER than any player figure base
- There is only ONE ball on the pitch

WHAT PLAYER FIGURES LOOK LIKE (do NOT confuse with the ball):
- Plastic figures standing on large flat circular bases (~2-3 cm diameter)
- The bases are flat discs pressed against the felt
- Much larger than the ball
- There are many of them arranged across the pitch

WHAT COUNTS AS A GOAL:
- The ball must be INSIDE the goal net, behind the goal line
- The goal is a rectangular frame/net at either SHORT end of the pitch
- The ball must be visibly past the goal posts/crossbar, inside the net structure
- A ball sitting IN FRONT of the goal (between the goal and the pitch) does NOT count
- Human hands may be visible — ignore them completely

IMPORTANT: If you can see the ball clearly in the middle of the pitch or near players, that is NOT a goal.
Only answer true if the ball is visibly inside a goal net.

Respond ONLY with valid JSON, no other text:
{"inGoal": true or false, "confidence": "high", "medium", or "low"}`;

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
          { type: 'text',  text: VISION_PROMPT }
        ]
      }]
    })
  });

  const data = await upstream.json();
  res.status(upstream.status).json(data);
};
