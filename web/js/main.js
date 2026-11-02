/* ============================================================================
   FAFNIR — WEBSITE BEHAVIOUR
   Navigation toggle, scroll-spy, reveal-on-scroll, and code-copy affordance.
   ============================================================================ */

(function () {
  'use strict';

  /* ------------------------------------------------------------------------
     Mobile navigation toggle
     ------------------------------------------------------------------------ */
  const navToggle = document.getElementById('navToggle');
  const globalNavLinks = document.getElementById('globalNavLinks');

  if (navToggle && globalNavLinks) {
    navToggle.addEventListener('click', () => {
      const isOpen = globalNavLinks.classList.toggle('is-open');
      navToggle.setAttribute('aria-expanded', String(isOpen));
      navToggle.setAttribute('aria-label', isOpen ? 'Close menu' : 'Open menu');
    });

    globalNavLinks.addEventListener('click', (event) => {
      if (event.target instanceof Element && event.target.tagName === 'A') {
        globalNavLinks.classList.remove('is-open');
        navToggle.setAttribute('aria-expanded', 'false');
        navToggle.setAttribute('aria-label', 'Open menu');
      }
    });
  }

  /* ------------------------------------------------------------------------
     Reveal on scroll (IntersectionObserver)
     ------------------------------------------------------------------------ */
  const revealTargets = document.querySelectorAll('.reveal');

  const revealObserver = new IntersectionObserver(
    (entries) => {
      entries.forEach((entry) => {
        if (entry.isIntersecting) {
          entry.target.classList.add('is-visible');
          revealObserver.unobserve(entry.target);
        }
      });
    },
    { rootMargin: '0px 0px -8% 0px', threshold: 0.05 }
  );

  revealTargets.forEach((element) => revealObserver.observe(element));

  /* ------------------------------------------------------------------------
     Scroll-spy for the frosted sub-navigation
     ------------------------------------------------------------------------ */
  const subNavAnchors = document.querySelectorAll('#subNavLinks a');
  const trackedSections = new Map();

  subNavAnchors.forEach((anchor) => {
    const hash = anchor.getAttribute('href');
    if (!hash || !hash.startsWith('#')) return;
    const section = document.querySelector(hash);
    if (section) trackedSections.set(section.id, anchor);
  });

  const spyObserver = new IntersectionObserver(
    (entries) => {
      entries.forEach((entry) => {
        const anchor = trackedSections.get(entry.target.id);
        if (!anchor) return;
        if (entry.isIntersecting) {
          subNavAnchors.forEach((link) => link.classList.remove('is-active'));
          anchor.classList.add('is-active');
        }
      });
    },
    { rootMargin: '-35% 0px -55% 0px' }
  );

  trackedSections.forEach((_anchor, id) => {
    const section = document.getElementById(id);
    if (section) spyObserver.observe(section);
  });

  /* ------------------------------------------------------------------------
     Copy-to-clipboard on code windows
     ------------------------------------------------------------------------ */
  document.querySelectorAll('.code-window').forEach((windowEl) => {
    const button = windowEl.querySelector('.copy-btn');
    const pre = windowEl.querySelector('.code-window__body');
    if (!button || !pre) return;

    let resetTimer = null;

    button.addEventListener('click', async () => {
      try {
        await navigator.clipboard.writeText(pre.textContent.trim());
        button.textContent = 'Copied';
      } catch {
        button.textContent = 'Error';
      }

      clearTimeout(resetTimer);
      resetTimer = setTimeout(() => {
        button.textContent = 'Copy';
      }, 1800);
    });
  });
})();
