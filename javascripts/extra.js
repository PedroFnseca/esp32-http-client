(function() {
    var storedLang = sessionStorage.getItem("user_lang_preference");
    
    document.addEventListener("click", function(e) {
        var link = e.target.closest("a[href*='/pt/'], a[href$='/pt'], a.md-select__link");
        if (link) {
            var href = link.getAttribute("href") || "";
            if (href.includes("/pt/") || href.endsWith("/pt")) {
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
        var isPtPath = currentPath.includes("/pt/") || currentPath.endsWith("/pt");

        if (isPortuguese && !isPtPath) {
            sessionStorage.setItem("user_lang_preference", "pt");
            var targetPath = currentPath;
            
            if (targetPath.endsWith("/")) {
                targetPath = targetPath + "pt/";
            } else {
                var lastSlash = targetPath.lastIndexOf("/");
                targetPath = targetPath.substring(0, lastSlash) + "/pt" + targetPath.substring(lastSlash);
            }
            
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

    var isPtPage = window.location.pathname.includes("/pt/") || window.location.pathname.endsWith("/pt");
    
    function filterSearchContainer(container) {
        if (!container) return;
        var links = container.querySelectorAll("a.md-search-result__link, .md-search-result__item a");
        links.forEach(function(link) {
            var href = link.getAttribute("href") || "";
            var isPtLink = href.includes("/pt/") || href.startsWith("pt/") || href.startsWith("../pt/");
            
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
