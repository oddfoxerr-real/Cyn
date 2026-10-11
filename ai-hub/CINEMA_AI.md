# Cyn Cinema AI — experimental movie/series profile

Cyn Cinema AI is published in the **native Cyn AI Hub**. In Cyn 2.0.16 or newer, open **AI Hub → Refresh community**, choose **Cyn Cinema AI**, then select **Import selected AI**. It downloads `cyn-cinema.cynai.json` from this repository; no app update or separate AI website is required.

This is a separate importable **neural profile**, not an automatic genre detector. It can be used for films, TV episodes, documentaries, streaming video, and other dialogue-containing content. Its targets emphasize dialogue-associated frequency balance and keep the Bass response away from the 100%-more extreme. **Only Clarity and Bass heads are trained**; Ambience, Surround and Dynamic Boost still need More/Less feedback.

Training generates synthetic eleven-dimensional spectral features and heuristic preference targets, with independent validation and held-out synthetic tests. This does **not** prove dialogue intelligibility, movie listening quality, speech recognition, or performance on specific streaming sources. See `cyn-cinema-evaluation.json` after training for actual synthetic metrics.

Importing an AI replaces the currently active personal neural profile after a local backup, so export your music profile if you want to switch back later. The built-in Cyn Starter remains available.
