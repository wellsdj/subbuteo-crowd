const { test, expect } = require('@playwright/test');
const { gotoApp } = require('./helpers');

// Note: const/let variables in the page script are accessible bare (not via
// window.XXX) inside page.evaluate(). Only var declarations and function
// declarations appear as properties of window.

test.describe('Pure logic units', () => {
  test('isBuiltInCamera returns true for FaceTime', async ({ page }) => {
    await gotoApp(page);
    const result = await page.evaluate(() => isBuiltInCamera('FaceTime HD Camera'));
    expect(result).toBe(true);
  });

  test('isBuiltInCamera returns true for Integrated', async ({ page }) => {
    await gotoApp(page);
    const result = await page.evaluate(() => isBuiltInCamera('Integrated Webcam'));
    expect(result).toBe(true);
  });

  test('isBuiltInCamera returns false for external USB cam', async ({ page }) => {
    await gotoApp(page);
    const result = await page.evaluate(() => isBuiltInCamera('Logitech C920 USB Camera'));
    expect(result).toBe(false);
  });

  test('isBuiltInCamera returns false for empty string', async ({ page }) => {
    await gotoApp(page);
    const result = await page.evaluate(() => isBuiltInCamera(''));
    expect(result).toBe(false);
  });

  test('isBuiltInCamera is case-insensitive for facetime', async ({ page }) => {
    await gotoApp(page);
    const result = await page.evaluate(() => isBuiltInCamera('facetime hd camera'));
    expect(result).toBe(true);
  });

  test('TIMER_TOTAL is 1800 seconds (30 minutes)', async ({ page }) => {
    await gotoApp(page);
    const total = await page.evaluate(() => TIMER_TOTAL);
    expect(total).toBe(1800);
  });

  test('timerSeconds starts at TIMER_TOTAL', async ({ page }) => {
    await gotoApp(page);
    const secs = await page.evaluate(() => timerSeconds);
    expect(secs).toBe(1800);
  });

  test('timerRunning starts false', async ({ page }) => {
    await gotoApp(page);
    const running = await page.evaluate(() => timerRunning);
    expect(running).toBe(false);
  });

  test('homeScore starts at 0', async ({ page }) => {
    await gotoApp(page);
    const score = await page.evaluate(() => homeScore);
    expect(score).toBe(0);
  });

  test('awayScore starts at 0', async ({ page }) => {
    await gotoApp(page);
    const score = await page.evaluate(() => awayScore);
    expect(score).toBe(0);
  });

  test('hlPlaying starts false', async ({ page }) => {
    await gotoApp(page);
    const playing = await page.evaluate(() => hlPlaying);
    expect(playing).toBe(false);
  });

  test('hlCumHome starts 0', async ({ page }) => {
    await gotoApp(page);
    const score = await page.evaluate(() => hlCumHome);
    expect(score).toBe(0);
  });

  test('hlCumAway starts 0', async ({ page }) => {
    await gotoApp(page);
    const score = await page.evaluate(() => hlCumAway);
    expect(score).toBe(0);
  });

  test('replayIsGoal starts false', async ({ page }) => {
    await gotoApp(page);
    const val = await page.evaluate(() => replayIsGoal);
    expect(val).toBe(false);
  });

  test('replaySeqIdx starts at 0', async ({ page }) => {
    await gotoApp(page);
    const val = await page.evaluate(() => replaySeqIdx);
    expect(val).toBe(0);
  });

  test('hlMicMuted starts false', async ({ page }) => {
    await gotoApp(page);
    const val = await page.evaluate(() => hlMicMuted);
    expect(val).toBe(false);
  });

  test('pendingHighlightId starts null', async ({ page }) => {
    await gotoApp(page);
    const val = await page.evaluate(() => pendingHighlightId);
    expect(val).toBeNull();
  });

  test('changeScore home adds 1 to homeScore', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => changeScore('home', 1));
    const score = await page.evaluate(() => homeScore);
    expect(score).toBe(1);
  });

  test('changeScore clamps home at 0', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => changeScore('home', -99));
    const score = await page.evaluate(() => homeScore);
    expect(score).toBe(0);
  });

  test('VAR_SEGMENT_MS is 22000', async ({ page }) => {
    await gotoApp(page);
    const val = await page.evaluate(() => VAR_SEGMENT_MS);
    expect(val).toBe(22000);
  });

  test('VAR_PHASE_OFFSET_MS is 11000', async ({ page }) => {
    await gotoApp(page);
    const val = await page.evaluate(() => VAR_PHASE_OFFSET_MS);
    expect(val).toBe(11000);
  });
});
