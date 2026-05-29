const { test, expect } = require('@playwright/test');
const { gotoApp } = require('./helpers');

test.describe('Goal flow', () => {
  test('goalPopup is hidden initially', async ({ page }) => {
    await gotoApp(page);
    await expect(page.locator('#goalPopup')).not.toHaveClass(/visible/);
  });

  test('showGoalPopup makes it visible', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      window.replayIsGoal = true;
      document.getElementById('goalPopup').classList.add('visible');
    });
    await expect(page.locator('#goalPopup')).toHaveClass(/visible/);
  });

  test('dismissGoal hides goalPopup', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      document.getElementById('goalPopup').classList.add('visible');
    });
    await page.evaluate(() => dismissGoal());
    await expect(page.locator('#goalPopup')).not.toHaveClass(/visible/);
  });

  test('dismissGoal shows scorePrompt', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      document.getElementById('goalPopup').classList.add('visible');
    });
    await page.evaluate(() => dismissGoal());
    await expect(page.locator('#scorePrompt')).toHaveClass(/visible/);
  });

  test('score prompt has home, away, and no buttons', async ({ page }) => {
    await gotoApp(page);
    const homeBtn = page.locator('.score-prompt-btn.home');
    const awayBtn = page.locator('.score-prompt-btn.away');
    const noBtn   = page.locator('.score-prompt-btn.no');
    await expect(homeBtn).toBeAttached();
    await expect(awayBtn).toBeAttached();
    await expect(noBtn).toBeAttached();
  });

  test('clicking home in scorePrompt increments home score', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      window.pendingHighlightId = null;
      document.getElementById('scorePrompt').classList.add('visible');
    });
    await page.locator('.score-prompt-btn.home').click();
    await expect(page.locator('#msHomeScore')).toHaveText('1');
  });

  test('clicking away in scorePrompt increments away score', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      window.pendingHighlightId = null;
      document.getElementById('scorePrompt').classList.add('visible');
    });
    await page.locator('.score-prompt-btn.away').click();
    await expect(page.locator('#msAwayScore')).toHaveText('1');
  });

  test('clicking no in scorePrompt hides it without scoring', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      window.pendingHighlightId = null;
      document.getElementById('scorePrompt').classList.add('visible');
    });
    await page.locator('.score-prompt-btn.no').click();
    await expect(page.locator('#scorePrompt')).not.toHaveClass(/visible/);
    await expect(page.locator('#msHomeScore')).toHaveText('0');
  });

  test('scorePrompt hides after selecting team', async ({ page }) => {
    await gotoApp(page);
    await page.evaluate(() => {
      window.pendingHighlightId = null;
      document.getElementById('scorePrompt').classList.add('visible');
    });
    await page.locator('.score-prompt-btn.home').click();
    await expect(page.locator('#scorePrompt')).not.toHaveClass(/visible/);
  });
});
