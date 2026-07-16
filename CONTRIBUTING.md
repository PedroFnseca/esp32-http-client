# Contributing to ESP32-HTTP-Client

First of all, thank you so much for your interest in contributing to our project! 🎉

To keep our commit history organized and ensure our automated versioning and release system works flawlessly, we have adopted the **Conventional Commits** standard.

## Commit Rules

All your commits must strictly follow the format below:

```
<type>(<optional scope>): <short and objective description>
```

### Commit Types that Generate Changelog

Our project automation is configured to specifically track the types below. Whenever one of these types is used, it will **automatically appear** in our `CHANGELOG.md` file and will be included in the notes of the next official Release:

*   **`feat`**: A new feature or capability.
*   **`fix`**: A bug fix.
*   **`perf`**: A code change that improves performance (e.g., memory usage or speed).
*   **`docs`**: Documentation only changes (e.g., editing `README.md`, writing new guides).
*   **`chore`**: Maintenance tasks, build tools, infrastructure tweaks, dependency updates, and other modifications that don't directly affect the library's functionality for the user.
*   **`refactor`**: A code change that neither fixes a bug nor adds a feature (e.g., restructuring internal logic).
*   **`test`**: Adding missing tests or correcting existing tests.
*   **`style`**: Changes that do not affect the meaning of the code (white-space, formatting, missing semi-colons, etc).

*All of these 8 types are officially tracked and will bring your contributions to life in our project Releases.*

### Examples of Valid Commits

*   `feat: add native support for array JSON parsing`
*   `fix: resolve memory leak on persistent connections`
*   `docs: update usage examples in README`
*   `perf: improve read stream performance using internal buffer`
*   `chore: update github actions version`
*   `refactor: simplify request building logic`
*   `test: add unit tests for nested JSON extraction`
*   `style: format files using clang-format`

## Why is this important?

Whenever your code is merged into the `main` branch, our action will look for these keywords (`feat`, `fix`, etc.) and decide on its own if our library needs a `Patch` (1.0.x), `Minor` (1.x.0) or `Major` (x.0.0) version bump. It will seamlessly open a Release Pull Request generating the tags for us automatically.

Thank you very much for your time and contributions!
