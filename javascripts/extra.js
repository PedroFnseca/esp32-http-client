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
});
