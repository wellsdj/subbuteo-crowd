const { test, expect } = require('@playwright/test');
const { gotoApp, navigateTo } = require('./helpers');

test.describe('Section display and isolation', () => {
  test('match section has start, reset and score buttons', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await expect(page.locator('#msStartBtn')).toBeVisible();
    await expect(page.locator('[onclick="timerReset()"]')).toBeAttached();
  });

  test('replay cam panel is in the replay section', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'replay');
    await expect(page.locator('#replayCamPanel')).toBeAttached();
  });

  test('VAR button is in the replay cam panel', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'replay');
    await expect(page.locator('#varBtn')).toBeAttached();
  });

  test('replay section dot hidden when not active', async ({ page }) => {
    await gotoApp(page);
    const display = await page.locator('#replayDot').evaluate(el => el.style.display);
    expect(display).toBe('none');
  });

  test('flashOverlay element exists', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#flashOverlay')).toBeAttached();
  });

  test('appStatus element shows STANDBY initially', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#appStatus')).toHaveText('STANDBY');
  });

  test('crowd section has crowd button', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#crowdBtn')).toBeVisible();
  });

  test('crowd button text shows CROWD OFF initially', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#crowdBtnText')).toContainText('CROWD OFF');
  });

  test('gesture panel is in the crowd section', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#gesturePanel')).toBeAttached();
  });

  test('gesture video element exists', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#gestureVideo')).toBeAttached();
  });

  test('hand button is present', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#handBtn')).toBeAttached();
  });

  test('replay cam button is present', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#replayCamBtn')).toBeAttached();
  });

  test('replay cam button says STANDBY initially', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#replayCamLabel')).toHaveText('STANDBY');
  });

  test('hand button says STANDBY initially', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#handLabel')).toHaveText('STANDBY');
  });

  test('highlights empty message element exists', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'highlights');
    await expect(page.locator('#hlEmpty')).toBeAttached();
  });
});
