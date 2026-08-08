(function() {
    var storedLang = sessionStorage.getItem("user_lang_preference");
    
    document.addEventListener("click", function(e) {
        var link = e.target.closest("a[href*='/pt/'], a[href$='/pt'], a.md-select__link");
        if (link) {
            var href = link.getAttribute("href") || "";
            if (href.includes("/pt/") || href.endsWith("/pt") || /(?:^|\/)pt(?:\/|$)/.test(href)) {
                sessionStorage.setItem("user_lang_preference", "pt");
            } else {
                sessionStorage.setItem("user_lang_preference", "en");
            }
        }
    });

    if (!storedLang) {
        var userLang = (navigator.language || navigator.userLanguage || "").toLowerCase();
        var isPortuguese = userLang.startsWith("pt");
        var currentPath = window.location.pathname;
        var isPtPath = /(?:^|\/)pt(?:\/|$)/.test(currentPath);

        if (isPortuguese && !isPtPath) {
            sessionStorage.setItem("user_lang_preference", "pt");
            
            var knownBases = ["/docs", "/esp32-http-client"];
            var base = "";
            for (var i = 0; i < knownBases.length; i++) {
                if (currentPath === knownBases[i] || currentPath.startsWith(knownBases[i] + "/")) {
                    base = knownBases[i];
                    break;
                }
            }
            
            var relativePath = currentPath.slice(base.length);
            if (!relativePath.startsWith("/")) {
                relativePath = "/" + relativePath;
            }
            
            var targetPath = base + "/pt" + relativePath;
            if (currentPath.endsWith("/") && !targetPath.endsWith("/")) {
                targetPath += "/";
            }
            
            targetPath += window.location.search + window.location.hash;
            window.location.replace(targetPath);
        } else if (!isPortuguese) {
            sessionStorage.setItem("user_lang_preference", "en");
        }
    }
})();

document.addEventListener("DOMContentLoaded", function() {
    var navItems = document.querySelectorAll(".md-nav__item--nested");
    navItems.forEach(function(item) {
        var toggle = item.querySelector(".md-nav__toggle");
        if (toggle) {
            item.addEventListener("mouseenter", function() {
                toggle.checked = true;
            });
            item.addEventListener("mouseleave", function() {
                if (!item.classList.contains("md-nav__item--active")) {
                    toggle.checked = false;
                }
            });
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
});
