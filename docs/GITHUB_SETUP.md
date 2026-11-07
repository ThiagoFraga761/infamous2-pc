# GitHub settings (set once, by hand)

Description:

    inFAMOUS 2 native PC port: PS3 static recompilation to Linux, Vulkan renderer, up to 4K, unlocked fps, no emulator. Bring your own disc.

Topics:

    infamous-2 infamous ps3 ps3-recompilation static-recompilation ps3recomp vulkan pc-port linux native-port game-preservation recompiled rsx spu

Social preview: upload `docs/screenshots/social-preview.jpg` under Settings, General, Social preview.

## Releases

Each release is a tag on `main` plus a GitHub release with the notes from
`docs/RELEASE_NOTES_<version>.md`, marked as a pre-release while the version
is 0.x. Attach nothing: GitHub adds the source archives by itself, and a
build or any file from the game must never be uploaded.

    scripts/check_clean.sh --tracked
    git tag -a v0.1.0 -m "v0.1.0"
    git push github main v0.1.0
    gh release create v0.1.0 --repo ThiagoFraga761/infamous2-pc --prerelease \
        --title "v0.1.0 — primeiro release" --notes-file docs/RELEASE_NOTES_v0.1.0.md

Before tagging, `CHANGELOG.md` needs the version's entry and its link at the end.
