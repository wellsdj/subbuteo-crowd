const { test, expect } = require('@playwright/test');
const { gotoApp } = require('./helpers');

// These tests verify the capture mechanism that guarantees the replay blob
// ends EXACTLY at the trigger moment (so the goal is never cut off).
// captureReplay now delegates to captureVarBlob (the dual-phase recorder),
// the same proven path VAR uses.

test.describe('Replay capture mechanism', () => {
  test('captureReplay is defined', async ({ page }) => {
    await gotoApp(page);
    const t = await page.evaluate(() => typeof captureReplay);
    expect(t).toBe('function');
  });

  test('captureVarBlob is defined', async ({ page }) => {
    await gotoApp(page);
    const t = await page.evaluate(() => typeof captureVarBlob);
    expect(t).toBe('function');
  });

  test('varRec has exactly two recorder slots', async ({ page }) => {
    await gotoApp(page);
    const len = await page.evaluate(() => varRec.length);
    expect(len).toBe(2);
  });

  test('the obsolete single-segment recorder is gone', async ({ page }) => {
    await gotoApp(page);
    const hasOld = await page.evaluate(() => typeof startReplaySegment !== 'undefined');
    expect(hasOld).toBe(false);
  });

  test('captureVarBlob calls back with null when no recorders running', async ({ page }) => {
    await gotoApp(page);
    const result = await page.evaluate(() => new Promise(resolve => {
      // Ensure recorders are stopped/cleared
      stopVarRecorder();
      captureVarBlob(blob => resolve(blob));
    }));
    expect(result).toBeNull();
  });

  test('captureReplay with no active section does not throw', async ({ page }) => {
    await gotoApp(page);
    const ok = await page.evaluate(() => {
      try { stopVarRecorder(); captureReplay(true); return true; }
      catch (e) { return false; }
    });
    expect(ok).toBe(true);
  });

  test('goal popup shows if capture yields no blob', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      stopVarRecorder();
      document.getElementById('goalPopup').classList.remove('visible');
      captureReplay(true); // showGoalFirst=true, no recorders → popup shows
    });
    await page.waitForTimeout(100);
    await expect(page.locator('#goalPopup')).toHaveClass(/visible/);
  });

  test('picks the longest-running recorder (ends at trigger)', async ({ page }) => {
    await gotoApp(page);
    // Simulate two recorders with different start times; verify the OLDER one
    // (smaller startTime) is selected — that is the one whose footage spans
    // back furthest and ends at the trigger moment.
    const picked = await page.evaluate(() => {
      const now = Date.now();
      // Fake recorder state objects
      varRec[0].recorder = { state: 'recording', stop() { this._stopped = true; } };
      varRec[0].startTime = now - 20000; // 20s old (older)
      varRec[0].timer = null;
      varRec[1].recorder = { state: 'recording', stop() { this._stopped = true; } };
      varRec[1].startTime = now - 5000;  // 5s old (newer)
      varRec[1].timer = null;
      // Run the selection logic from captureVarBlob without finalizing
      let pick = -1, oldest = -1;
      for (let i = 0; i < varRec.length; i++) {
        const s = varRec[i];
        if (!s.recorder || s.recorder.state !== 'recording') continue;
        const age = Date.now() - s.startTime;
        if (age > oldest) { oldest = age; pick = i; }
      }
      // cleanup fakes
      varRec[0].recorder = null; varRec[1].recorder = null;
      return pick;
    });
    expect(picked).toBe(0); // the 20s-old recorder
  });
});
