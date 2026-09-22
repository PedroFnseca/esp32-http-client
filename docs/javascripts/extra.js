// Persist language and theme preferences across sessions
document.addEventListener("click", function(e) {
    // Language switcher detection
    var langLink = e.target.closest("a[href*='/pt/'], a[href$='/pt'], a.md-select__link");
    if (langLink) {
        var href = langLink.getAttribute("href") || "";
        var isPt = href.includes("/pt/") || href.endsWith("/pt") || /(?:^|\/)pt(?:\/|$)/.test(href);
        var lang = isPt ? "pt" : "en";
        try {
            localStorage.setItem("user_lang_preference", lang);
            sessionStorage.setItem("user_lang_preference", lang);
            sessionStorage.setItem("user_lang_checked", "1");
        } catch (err) {}
    }

    // Palette theme switcher detection
    var themeToggle = e.target.closest("form[data-md-component='palette'] label, input[name='__palette']");
    if (themeToggle) {
        setTimeout(function() {
            try {
                var currentScheme = document.body.getAttribute("data-md-color-scheme");
                if (currentScheme) {
                    localStorage.setItem("user_theme_preference", currentScheme);
                }
            } catch (err) {}
        }, 50);
    }
}, { passive: true });

// Live system theme sync if no explicit override is saved
if (window.matchMedia) {
    var darkModeQuery = window.matchMedia("(prefers-color-scheme: dark)");
    var handleThemeChange = function(e) {
        try {
            if (!localStorage.getItem("user_theme_preference")) {
                var newScheme = e.matches ? "slate" : "default";
                if (typeof __md_set === "function") {
                    __md_set("__palette", {
                        index: e.matches ? 0 : 1,
                        color: { scheme: newScheme, primary: "teal", accent: "orange" }
                    });
                }
                if (document.body) {
                    document.body.setAttribute("data-md-color-scheme", newScheme);
                }
            }
        } catch (err) {}
    };
    if (darkModeQuery.addEventListener) {
        darkModeQuery.addEventListener("change", handleThemeChange);
    } else if (darkModeQuery.addListener) {
        darkModeQuery.addListener(handleThemeChange);
    }
}

document.addEventListener("DOMContentLoaded", function() {
    var navItems = document.querySelectorAll(".md-nav__item--nested");
    navItems.forEach(function(item) {
        var toggle = item.querySelector(".md-nav__toggle");
        if (toggle) {
            item.addEventListener("mouseenter", function() {
                toggle.checked = true;
            }, { passive: true });
            item.addEventListener("mouseleave", function() {
                if (!item.classList.contains("md-nav__item--active")) {
                    toggle.checked = false;
                }
            }, { passive: true });
        }
    });

    var isPtPage = /(?:^|\/)pt(?:\/|$)/.test(window.location.pathname);
    
    function filterSearchContainer(container) {
        if (!container) return;
        var links = container.querySelectorAll("a.md-search-result__link, .md-search-result__item a");
        links.forEach(function(link) {
            var href = link.getAttribute("href") || "";
            var isPtLink = href.includes("/pt/") || href.startsWith("pt/") || href.startsWith("../pt/") || /(?:^|\/)pt(?:\/|$)/.test(href);
            var item = link.closest(".md-search-result__item") || link.parentElement;
            if (!item) return;

            if (isPtPage && !isPtLink) {
                item.style.display = "none";
            } else if (!isPtPage && isPtLink) {
                item.style.display = "none";
            } else {
                item.style.display = "";
            }
        });
    }

    var searchOutput = document.querySelector(".md-search__output, .md-search-result");
    if (searchOutput) {
        filterSearchContainer(searchOutput);
        var observer = new MutationObserver(function() {
            filterSearchContainer(searchOutput);
        });
        observer.observe(searchOutput, { childList: true, subtree: true });
    }

    initPrivacyConsent();
    initTabbedScrollIndicators();
});

if (typeof document$ !== "undefined") {
    document$.subscribe(initTabbedScrollIndicators);
}

function initPrivacyConsent() {
    var CONSENT_KEY = "esp32_doc_privacy_consent";
    if (localStorage.getItem(CONSENT_KEY)) {
        return;
    }

    var isPt = /(?:^|\/)pt(?:\/|$)/.test(window.location.pathname) || 
               sessionStorage.getItem("user_lang_preference") === "pt" ||
               (document.documentElement && document.documentElement.lang === "pt");

    var privacyUrl = isPt ? "/pt/privacy/" : "/privacy/";

    var contentPt = {
        title: "Privacidade e Métricas",
        message: "Utilizamos <strong>Microsoft Clarity</strong> e <strong>Google Analytics</strong> para entender o uso e aprimorar a documentação.",
        learnMore: "Saiba mais",
        accept: "Aceitar",
        decline: "Recusar"
    };

    var contentEn = {
        title: "Privacy & Analytics",
        message: "We use <strong>Microsoft Clarity</strong> and <strong>Google Analytics</strong> to analyze usage and improve the documentation.",
        learnMore: "Learn more",
        accept: "Accept",
        decline: "Decline"
    };

    var content = isPt ? contentPt : contentEn;

    var banner = document.createElement("div");
    banner.id = "privacy-consent-banner";
    banner.className = "privacy-consent-card";
    banner.setAttribute("role", "dialog");
    banner.setAttribute("aria-live", "polite");
    banner.setAttribute("aria-label", content.title);

    banner.innerHTML = [
        '<div class="privacy-consent-header">',
        '  <div class="privacy-consent-icon" aria-hidden="true">',
        '    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">',
        '      <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>',
        '    </svg>',
        '  </div>',
        '  <span class="privacy-consent-title">' + content.title + '</span>',
        '</div>',
        '<p class="privacy-consent-text">' + content.message + ' <a href="' + privacyUrl + '" class="privacy-consent-link">' + content.learnMore + '</a></p>',
        '<div class="privacy-consent-actions">',
        '  <button type="button" class="privacy-consent-btn privacy-consent-btn-secondary" id="privacy-consent-decline">' + content.decline + '</button>',
        '  <button type="button" class="privacy-consent-btn privacy-consent-btn-primary" id="privacy-consent-accept">' + content.accept + '</button>',
        '</div>'
    ].join("\n");

    document.body.appendChild(banner);

    var closeBanner = function(choice) {
        localStorage.setItem(CONSENT_KEY, choice);
        banner.classList.add("privacy-consent-closing");
        setTimeout(function() {
            if (banner.parentNode) {
                banner.parentNode.removeChild(banner);
            }
        }, 300);
    };

    var acceptBtn = document.getElementById("privacy-consent-accept");
    var declineBtn = document.getElementById("privacy-consent-decline");

    if (acceptBtn) {
        acceptBtn.addEventListener("click", function() {
            closeBanner("accepted");
        });
    }

    if (declineBtn) {
        declineBtn.addEventListener("click", function() {
            closeBanner("declined");
        });
    }
}

// Horizontal scroll indicators & smooth scrolling for tabbed sets
function initTabbedScrollIndicators() {
    var tabbedSets = document.querySelectorAll(".md-typeset .tabbed-set");
    tabbedSets.forEach(function(set) {
        var labels = set.querySelector(".tabbed-labels");
        if (!labels) return;

        var nextBtn = set.querySelector(".tab-scroll-next");
        var prevBtn = set.querySelector(".tab-scroll-prev");

        var isPt = /(?:^|\/)pt(?:\/|$)/.test(window.location.pathname) || 
                   sessionStorage.getItem("user_lang_preference") === "pt" ||
                   (document.documentElement && document.documentElement.lang === "pt");
        var moreText = isPt ? "Mais" : "More";

        if (!nextBtn) {
            nextBtn = document.createElement("button");
            nextBtn.className = "tab-scroll-btn tab-scroll-next";
            nextBtn.type = "button";
            nextBtn.setAttribute("aria-label", isPt ? "Rolar mais abas para a direita" : "Scroll more tabs right");
            nextBtn.innerHTML = '<span>' + moreText + '</span><span class="tab-scroll-arrow"><svg viewBox="0 0 24 24"><path d="M8.59 16.59L13.17 12 8.59 7.41 10 6l6 6-6 6-1.41-1.41z"/></svg></span>';
            set.appendChild(nextBtn);

            nextBtn.addEventListener("click", function(e) {
                e.preventDefault();
                labels.scrollBy({ left: 180, behavior: "smooth" });
            });
        }

        if (!prevBtn) {
            prevBtn = document.createElement("button");
            prevBtn.className = "tab-scroll-btn tab-scroll-prev";
            prevBtn.type = "button";
            prevBtn.setAttribute("aria-label", isPt ? "Rolar abas para a esquerda" : "Scroll tabs left");
            prevBtn.innerHTML = '<svg viewBox="0 0 24 24"><path d="M15.41 7.41L10.83 12l4.58 4.59L14 18l-6-6 6-6 1.41 1.41z"/></svg>';
            set.appendChild(prevBtn);

            prevBtn.addEventListener("click", function(e) {
                e.preventDefault();
                labels.scrollBy({ left: -180, behavior: "smooth" });
            });
        }

        function updateScrollState() {
            var maxScroll = labels.scrollWidth - labels.clientWidth;
            if (maxScroll > 6) {
                set.classList.add("has-horizontal-scroll");
                if (labels.scrollLeft < maxScroll - 6) {
                    set.classList.add("can-scroll-right");
                } else {
                    set.classList.remove("can-scroll-right");
                }
                if (labels.scrollLeft > 6) {
                    set.classList.add("can-scroll-left");
                } else {
                    set.classList.remove("can-scroll-left");
                }
            } else {
                set.classList.remove("has-horizontal-scroll", "can-scroll-right", "can-scroll-left");
            }
        }

        labels.addEventListener("scroll", updateScrollState, { passive: true });
        window.addEventListener("resize", updateScrollState, { passive: true });

        labels.querySelectorAll("label").forEach(function(lbl) {
            lbl.addEventListener("click", function() {
                setTimeout(function() {
                    lbl.scrollIntoView({ behavior: "smooth", block: "nearest", inline: "center" });
                    updateScrollState();
                }, 60);
            });
        });

        setTimeout(updateScrollState, 100);
        setTimeout(updateScrollState, 500);
    });
}

