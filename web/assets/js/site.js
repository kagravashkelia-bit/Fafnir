/* ============================================================================
   FAFNIR — SITE INTERACTIONS
   Small, dependency-free enhancements. Every feature degrades gracefully when
   JavaScript is unavailable: navigation remains ordinary links, code blocks
   remain readable, and documentation stays searchable by the browser's own
   find-in-page.
   ========================================================================== */
(function () {
  "use strict";

  var doc = document;
  var root = doc.documentElement;

  /* --------------------------------------------------------------------------
     Header state + mobile navigation
     ------------------------------------------------------------------------ */
  var header = doc.querySelector(".site-header");
  var navToggle = doc.getElementById("navToggle");
  var mobileNav = doc.getElementById("mobileNav");

  function onScroll() {
    if (!header) return;
    header.classList.toggle("is-scrolled", window.scrollY > 4);
  }
  onScroll();
  window.addEventListener("scroll", onScroll, { passive: true });

  function setMenu(open) {
    if (!navToggle || !mobileNav) return;
    navToggle.setAttribute("aria-expanded", open ? "true" : "false");
    navToggle.setAttribute("aria-label", open ? "Close menu" : "Open menu");
    mobileNav.classList.toggle("is-open", open);
    doc.body.classList.toggle("nav-open", open);
    if (open) {
      var first = mobileNav.querySelector("a, button");
      if (first) first.focus({ preventScroll: true });
    }
  }

  if (navToggle && mobileNav) {
    navToggle.addEventListener("click", function () {
      setMenu(navToggle.getAttribute("aria-expanded") !== "true");
    });

    mobileNav.addEventListener("click", function (event) {
      if (event.target.closest("a")) setMenu(false);
    });

    doc.addEventListener("keydown", function (event) {
      if (event.key === "Escape") {
        if (navToggle.getAttribute("aria-expanded") === "true") {
          setMenu(false);
          navToggle.focus();
        }
        closeMenus(null);
      }
    });

    var media = window.matchMedia("(min-width: 861px)");
    var onChange = function (event) {
      if (event.matches) setMenu(false);
    };
    if (media.addEventListener) media.addEventListener("change", onChange);
    else if (media.addListener) media.addListener(onChange);
  }

  /* --------------------------------------------------------------------------
     Header "More" dropdown (<details>) — one open at a time, close on outside
     click or Escape.
     ------------------------------------------------------------------------ */
  var menus = Array.prototype.slice.call(doc.querySelectorAll(".nav-menu"));

  function closeMenus(except) {
    menus.forEach(function (menu) {
      if (menu !== except) menu.removeAttribute("open");
    });
  }

  menus.forEach(function (menu) {
    menu.addEventListener("toggle", function () {
      if (menu.open) closeMenus(menu);
    });
    menu.addEventListener("click", function (event) {
      if (event.target.closest("a")) menu.removeAttribute("open");
    });
  });

  doc.addEventListener("click", function (event) {
    if (!event.target.closest(".nav-menu")) closeMenus(null);
  });

  /* --------------------------------------------------------------------------
     Code copy buttons
     ------------------------------------------------------------------------ */
  function copyText(text) {
    if (navigator.clipboard && navigator.clipboard.writeText) {
      return navigator.clipboard.writeText(text);
    }
    return new Promise(function (resolve, reject) {
      var area = doc.createElement("textarea");
      area.value = text;
      area.setAttribute("readonly", "");
      area.style.position = "fixed";
      area.style.opacity = "0";
      doc.body.appendChild(area);
      area.select();
      try {
        var ok = doc.execCommand("copy");
        doc.body.removeChild(area);
        ok ? resolve() : reject(new Error("copy failed"));
      } catch (error) {
        doc.body.removeChild(area);
        reject(error);
      }
    });
  }

  Array.prototype.forEach.call(doc.querySelectorAll("[data-copy]"), function (button) {
    button.addEventListener("click", function () {
      var block = button.closest(".code");
      var code = block ? block.querySelector("code") : null;
      if (!code) return;
      var original = button.textContent;
      copyText(code.innerText.replace(/\u00a0/g, " ")).then(
        function () {
          button.textContent = "Copied";
          button.classList.add("is-copied");
          window.setTimeout(function () {
            button.textContent = original;
            button.classList.remove("is-copied");
          }, 1600);
        },
        function () {
          button.textContent = "Press Ctrl+C";
          window.setTimeout(function () {
            button.textContent = original;
          }, 2000);
        }
      );
    });
  });

  /* --------------------------------------------------------------------------
     Documentation sidebar search: filters page links by title.
     ------------------------------------------------------------------------ */
  var docsSearch = doc.getElementById("docsSearch");
  if (docsSearch) {
    var docsNoResults = doc.getElementById("docsNoResults");
    var docsLinks = Array.prototype.slice.call(doc.querySelectorAll(".docs-nav__link"));

    var filterDocs = function () {
      var query = docsSearch.value.trim().toLowerCase();
      var visible = 0;

      docsLinks.forEach(function (link) {
        var title = (link.textContent || "").toLowerCase();
        var group = link.closest(".docs-nav__group");
        var label = group ? group.querySelector(".docs-nav__label") : null;
        var groupTitle = label ? (label.textContent || "").toLowerCase() : "";
        var match = !query || title.indexOf(query) !== -1 || groupTitle.indexOf(query) !== -1;
        link.classList.toggle("is-hidden", !match);
        if (match) visible += 1;
        if (group) {
          var any = Array.prototype.some.call(
            group.querySelectorAll(".docs-nav__link"),
            function (item) {
              return !item.classList.contains("is-hidden");
            }
          );
          group.classList.toggle("is-empty", !any);
        }
      });

      if (docsNoResults) docsNoResults.classList.toggle("is-visible", visible === 0);
    };

    docsSearch.addEventListener("input", filterDocs);
    docsSearch.addEventListener("keydown", function (event) {
      if (event.key === "Escape") {
        docsSearch.value = "";
        filterDocs();
        docsSearch.blur();
      }
      if (event.key === "Enter") {
        var first = docsLinks.filter(function (link) {
          return !link.classList.contains("is-hidden");
        })[0];
        if (first) first.click();
      }
    });
  }

  /* --------------------------------------------------------------------------
     Changelog / list filters: [data-filter-group] buttons toggle entries by
     [data-tag] sections.
     ------------------------------------------------------------------------ */
  Array.prototype.forEach.call(doc.querySelectorAll("[data-filter-group]"), function (bar) {
    var buttons = Array.prototype.slice.call(bar.querySelectorAll("[data-filter]"));
    var scope = bar.parentElement || doc;
    var entries = Array.prototype.slice.call(scope.querySelectorAll("[data-entry]"));
    var empty = scope.querySelector("[data-filter-empty]");

    var apply = function (value) {
      var shown = 0;

      entries.forEach(function (entry) {
        var groups = Array.prototype.slice.call(entry.querySelectorAll("[data-tag]"));
        var visibleInEntry = 0;

        groups.forEach(function (group) {
          var match = value === "all" || group.getAttribute("data-tag") === value;
          group.hidden = !match;
          if (match) visibleInEntry += 1;
        });

        var entryVisible =
          groups.length === 0 ? value === "all" : visibleInEntry > 0;
        entry.hidden = !entryVisible;
        if (entryVisible) shown += 1;
      });

      if (empty) empty.classList.toggle("is-visible", shown === 0);
    };

    buttons.forEach(function (button) {
      button.addEventListener("click", function () {
        buttons.forEach(function (other) {
          other.classList.toggle("is-active", other === button);
          other.setAttribute("aria-pressed", other === button ? "true" : "false");
        });
        apply(button.getAttribute("data-filter") || "all");
      });
    });

    buttons.forEach(function (button) {
      button.setAttribute(
        "aria-pressed",
        button.classList.contains("is-active") ? "true" : "false"
      );
    });
  });

  /* --------------------------------------------------------------------------
     Table of contents scrollspy
     ------------------------------------------------------------------------ */
  var tocLinks = Array.prototype.slice.call(doc.querySelectorAll(".docs-toc a"));
  if (tocLinks.length) {
    var targets = tocLinks
      .map(function (link) {
        var id = link.getAttribute("href").slice(1);
        var el = doc.getElementById(id);
        return el ? { link: link, el: el } : null;
      })
      .filter(Boolean);

    var setActive = function (link) {
      tocLinks.forEach(function (item) {
        item.classList.toggle("is-active", item === link);
      });
    };

    if ("IntersectionObserver" in window) {
      var seen = new Map();
      var observer = new IntersectionObserver(
        function (entries) {
          entries.forEach(function (entry) {
            seen.set(entry.target, entry.isIntersecting ? entry.intersectionRatio : 0);
          });
          var best = null;
          var bestRatio = 0;
          targets.forEach(function (item) {
            var ratio = seen.get(item.el) || 0;
            if (ratio > bestRatio) {
              bestRatio = ratio;
              best = item;
            }
          });
          if (best) setActive(best.link);
        },
        { rootMargin: "-88px 0px -55% 0px", threshold: [0, 0.15, 0.4, 0.75, 1] }
      );
      targets.forEach(function (item) {
        observer.observe(item.el);
      });
    } else {
      var onSpy = function () {
        var offset = window.scrollY + 120;
        var current = targets[0];
        targets.forEach(function (item) {
          if (item.el.offsetTop <= offset) current = item;
        });
        if (current) setActive(current.link);
      };
      onSpy();
      window.addEventListener("scroll", onSpy, { passive: true });
    }
  }

  /* --------------------------------------------------------------------------
     Reading progress bar (docs + legal pages)
     ------------------------------------------------------------------------ */
  var progress = doc.getElementById("readingProgress");
  if (progress) {
    var updateProgress = function () {
      var scrollable = root.scrollHeight - root.clientHeight;
      var ratio = scrollable > 0 ? (window.scrollY / scrollable) * 100 : 0;
      progress.style.width = Math.min(100, Math.max(0, ratio)) + "%";
    };
    updateProgress();
    window.addEventListener("scroll", updateProgress, { passive: true });
    window.addEventListener("resize", updateProgress);
  }
})();
