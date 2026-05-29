const { test, expect } = require('@playwright/test');
const { gotoApp, navigateTo } = require('./helpers');

test.describe('Match timer', () => {
  test('timer starts at 20:00', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await expect(page.locator('#msTimer')).toHaveText('20:00');
  });

  test('start button exists', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await expect(page.locator('#msStartBtn')).toBeVisible();
    await expect(page.locator('#msStartBtn')).toHaveText('START');
  });

  test('start button changes to PAUSE when clicked', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await page.click('#msStartBtn');
    await expect(page.locator('#msStartBtn')).toHaveText('PAUSE');
  });

  test('timer decrements after starting', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await page.click('#msStartBtn');
    await page.waitForTimeout(2200);
    const text = await page.locator('#msTimer').textContent();
    expect(text).not.toBe('20:00');
  });

  test('pause stops countdown', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await page.click('#msStartBtn'); // start
    await page.waitForTimeout(1500);
    await page.click('#msStartBtn'); // pause
    const snapshot = await page.locator('#msTimer').textContent();
    await page.waitForTimeout(1500);
    const after = await page.locator('#msTimer').textContent();
    expect(after).toBe(snapshot);
  });

  test('reset returns timer to 20:00', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await page.click('#msStartBtn');
    await page.waitForTimeout(1500);
    await page.click('[onclick="timerReset()"]');
    await expect(page.locator('#msTimer')).toHaveText('20:00');
  });

  test('reset changes button back to START', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await page.click('#msStartBtn');
    await page.click('[onclick="timerReset()"]');
    await expect(page.locator('#msStartBtn')).toHaveText('START');
  });

  test('progress bar at 100% at start', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    const width = await page.locator('#msProgressBar').evaluate(el => el.style.width);
    expect(width).toBe('100%');
  });

  test('banner timer mirrors match timer', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await page.click('#msStartBtn');
    await page.waitForTimeout(1500);
    const matchTimer = await page.locator('#msTimer').textContent();
    const bannerTimer = await page.locator('#bannerTimer').textContent();
    expect(matchTimer).toBe(bannerTimer);
  });

  test('timer does not go below 00:00', async ({ page }) => {
    await gotoApp(page);
    await navigateTo(page, 'match');
    await page.evaluate(() => { timerSeconds = 1; });
    await page.click('#msStartBtn');
    await page.waitForTimeout(2500);
    const text = await page.locator('#msTimer').textContent();
    expect(text).toBe('00:00');
  });
});
