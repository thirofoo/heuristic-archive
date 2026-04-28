# AHC038 memo

## 考察

- 移動 ⇒ Catch ⇒ 移動 ⇒ Release の繰り返しだが、Catch の前とリリースの後に、他の移動することで得を得そうなたこ焼きも一気に移動させる

  - 毎回往復するのが無駄なので、一気に移動させることで効率化できる

- 今はたこ焼きを移動させる順番を固定させてるけど、固定させる意味がない
  - その都度、一番近いたこ焼きを選ぶようにすれば、より効率的に移動できる

# Todo

- まとまってるケースのみを通してどれくらいかを確認してみたい
- 先端に型を導入して、一気に持っていけるところは持っていくようにしてみる

custom_solver1.custom_arm_length = {1, 2, 4, 8, min(16, (custom_solver1.N + 1) / 2)};
custom_solver1.custom_arms = {{0,  1}, {1,  2}, {2,  4}, {3,  8}, {4, 16}, {4, 16}, {5,  1}, {5,  2}, {5,  3}, {5,  4}, {5,  5}, {5,  6}, {5,  7}, {5,  8}};
custom_solver1.custom_leafs = {{0,  0}, {0, -1}, {0, -2}, {0, -3}, {0, -4}, {0, -5}, {0, -6}, {0, -7}, {0, -8}};
custom_solver1.custom_first_op = {".LLLLLLLL", ".LLLLLLLL"};

custom_solver2.custom_arm_length = {min(16, (custom_solver2.N + 1) / 2), 8, 4, 2, 1};
custom_solver2.custom_arms = {{0, 16}, {1, 8}, {2, 4}, {3, 2}, {4, 1}, {4, 1}, {5, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 5}, {5, 6}, {5, 7}, {5, 8}};
custom_solver2.custom_leafs = {{0, 0}, {0, -1}, {0, -2}, {0, -3}, {0, -4}, {0, -5}, {0, -6}, {0, -7}, {0, -8}};
custom_solver2.custom_first_op = {".LLLLLLLL", ".LLLLLLLL"};

custom_solver1.custom_arm_length =
custom_solver1.custom_arms =
custom_solver1.custom_leafs =
custom_solver1.custom_first_op =
