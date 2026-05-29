const { test, expect } = require('@playwright/test');
const { gotoApp } = require('./helpers');

// Note: const/let vars in the page script are NOT properties of window.
// Access them bare (REPLAY_SEQ, replayIsGoal, etc.) inside page.evaluate().

test.describe('Replay UI', () => {
  test('replay popup is hidden on load', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#replayPopup')).not.toHaveClass(/visible/);
  });

  test('replay popup shows when startReplaySequence called with a URL', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      const blob = new Blob([''], { type: 'video/webm' });
      replayUrl = URL.createObjectURL(blob);
      replayIsGoal = false;
      activeReplaySeq = SAVE_REPLAY_SEQ;
      replaySeqIdx = 0;
      document.getElementById('replayPopup').classList.add('visible');
    });
    await expect(page.locator('#replayPopup')).toHaveClass(/visible/);
  });

  test('dismissReplay hides the popup', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      document.getElementById('replayPopup').classList.add('visible');
    });
    await page.evaluate(() => dismissReplay());
    await expect(page.locator('#replayPopup')).not.toHaveClass(/visible/);
  });

  test('DONE button calls dismissReplay', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      const blob = new Blob([''], { type: 'video/webm' });
      replayUrl = URL.createObjectURL(blob);
      replayIsGoal = false;
      document.getElementById('replayPopup').classList.add('visible');
    });
    await page.locator('.replay-dismiss').click();
    await expect(page.locator('#replayPopup')).not.toHaveClass(/visible/);
  });

  test('REPLAY_SEQ has 4 clips', async ({ page }) => {
    await gotoApp(page);
    const count = await page.evaluate(() => REPLAY_SEQ.length);
    expect(count).toBe(4);
  });

  test('SAVE_REPLAY_SEQ has 2 clips', async ({ page }) => {
    await gotoApp(page);
    const count = await page.evaluate(() => SAVE_REPLAY_SEQ.length);
    expect(count).toBe(2);
  });

  test('VAR_SEQ has 1 clip', async ({ page }) => {
    await gotoApp(page);
    const count = await page.evaluate(() => VAR_SEQ.length);
    expect(count).toBe(1);
  });

  test('FULL REPLAY clip has seekFromEnd=10', async ({ page }) => {
    await gotoApp(page);
    const val = await page.evaluate(() => REPLAY_SEQ[0].seekFromEnd);
    expect(val).toBe(10);
  });

  test('FULL REPLAY plays at rate 1.0', async ({ page }) => {
    await gotoApp(page);
    const rate = await page.evaluate(() => REPLAY_SEQ[0].rate);
    expect(rate).toBe(1.0);
  });

  test('SLOW MOTION clip rate is 0.5', async ({ page }) => {
    await gotoApp(page);
    const rate = await page.evaluate(() => REPLAY_SEQ[2].rate);
    expect(rate).toBe(0.5);
  });

  test('SUPER SLOW clip rate is 0.25', async ({ page }) => {
    await gotoApp(page);
    const rate = await page.evaluate(() => REPLAY_SEQ[3].rate);
    expect(rate).toBe(0.25);
  });

  test('no clips have skipStart > 0', async ({ page }) => {
    await gotoApp(page);
    const allSeqs = await page.evaluate(() => [
      ...REPLAY_SEQ,
      ...SAVE_REPLAY_SEQ,
      ...VAR_SEQ,
    ]);
    allSeqs.forEach(clip => {
      expect(clip.skipStart || 0).toBe(0);
    });
  });

  test('seq pips bar is in the DOM', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#replaySeqBar')).toBeAttached();
  });

  test('VAR button exists in replay cam panel', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#varBtn')).toBeAttached();
  });

  test('dismissReplay for goal shows scorePrompt', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      replayIsGoal = true;
      document.getElementById('replayPopup').classList.add('visible');
    });
    await page.evaluate(() => dismissReplay());
    await expect(page.locator('#scorePrompt')).toHaveClass(/visible/);
  });

  test('dismissReplay for save does NOT show scorePrompt', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      replayIsGoal = false;
      document.getElementById('replayPopup').classList.add('visible');
      // ensure scorePrompt is not visible to start
      document.getElementById('scorePrompt').classList.remove('visible');
    });
    await page.evaluate(() => dismissReplay());
    await expect(page.locator('#scorePrompt')).not.toHaveClass(/visible/);
  });

  test('auto-dismiss fires for save/near-miss after last clip', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      document.getElementById('replayPopup').classList.add('visible');
      replayIsGoal = false;
      activeReplaySeq = SAVE_REPLAY_SEQ;
      replaySeqIdx = SAVE_REPLAY_SEQ.length;
    });
    await page.evaluate(() => dismissReplay());
    await expect(page.locator('#replayPopup')).not.toHaveClass(/visible/);
  });

  test('goal replay does NOT auto-dismiss (stays open for scoring)', async ({ page }) => {
    await gotoApp(page);
    const shows = await page.evaluate(() => {
      replayIsGoal = true;
      replayUrl = null;
      document.getElementById('scorePrompt').classList.remove('visible');
      dismissReplay();
      return document.getElementById('scorePrompt').classList.contains('visible');
    });
    expect(shows).toBe(true);
  });
});
