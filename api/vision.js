const GOAL_PROMPT = `The camera is pointing at a Subbuteo miniature football goal from close range.

Answer two separate questions:

QUESTION 1 — ballSeen: Can you see the small round ball anywhere in the image at all?
- true if the ball is visible anywhere, even in front of or beside the goal
- false if no ball is visible

QUESTION 2 — inGoal: Is the ball physically inside the goal net, behind the goal line?
- true ONLY if the ball is sitting inside the net, clearly past the goal posts
- false if the ball is in FRONT of the goal, even touching the posts
- false if the ball is to the side of the goal
- false if you can see the ball but it has not crossed the goal line
- IMPORTANT: seeing the ball does NOT mean it is in the goal. Most of the time the ball will be visible but NOT in the goal.

Respond ONLY with valid JSON, no other text:
{"ballSeen": true or false, "inGoal": true or false, "confidence": "high" or "medium" or "low", "observation": "one short sentence"}`;

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
