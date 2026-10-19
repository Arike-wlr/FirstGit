"""生成桌面快捷方式用的 app.ico —— 图案与 index.html 的 favicon 一致。

favicon 设计（viewBox 0 0 32 32）：
  红色圆角矩形底 + 白色书本轮廓 + 两条横线

所有几何判定都在 32×32 的逻辑坐标系里做，
再按 k = n/32 映射到超采样画布，避免尺度混用。
"""
from PIL import Image
import os

SS = 8  # 超采样倍数

BRAND = (255, 36, 66)     # #ff2442
BRAND2 = (255, 108, 130)  # 渐变末端
WHITE = (255, 255, 255)

LOGICAL = 32.0  # 逻辑坐标系边长


def rounded_rect_sdf(x, y, x0, y0, x1, y1, r):
    """圆角矩形 SDF（矩形由对角点给出），<=0 表示在内部。"""
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    hw, hh = (x1 - x0) / 2, (y1 - y0) / 2
    qx = abs(x - cx) - (hw - r)
    qy = abs(y - cy) - (hh - r)
    ax, ay = max(qx, 0.0), max(qy, 0.0)
    return (ax * ax + ay * ay) ** 0.5 + min(max(qx, qy), 0.0) - r


def seg_dist(px, py, x1, y1, x2, y2):
    vx, vy = x2 - x1, y2 - y1
    wx, wy = px - x1, py - y1
    denom = vx * vx + vy * vy
    t = 0.0 if denom == 0 else max(0.0, min(1.0, (wx * vx + wy * vy) / denom))
    dx, dy = wx - t * vx, wy - t * vy
    return (dx * dx + dy * dy) ** 0.5


# —— favicon 里的几何参数（逻辑坐标）——
OUTER = (0.0, 0.0, 32.0, 32.0, 9.0)          # 外层圆角底
BOOK = (8.0, 8.5, 24.0, 23.5, 4.0)           # 书本轮廓
STROKE = 2.1                                  # 描边宽度
LINES = [                                      # 两条横线
    (12.0, 14.0, 20.0, 14.0),
    (12.0, 18.2, 17.0, 18.2),
]


def render(size):
    n = size * SS
    img = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    px = img.load()

    half_stroke = STROKE / 2
    half_line = STROKE / 2

    for py in range(n):
        y = (py + 0.5) * LOGICAL / n
        for pxi in range(n):
            x = (pxi + 0.5) * LOGICAL / n

            # 外层圆角底
            if rounded_rect_sdf(x, y, *OUTER) > 0:
                continue

            # 底色渐变（左上偏红 → 右下偏粉）
            t = max(0.0, min(1.0, (x / LOGICAL * 0.45 + y / LOGICAL * 0.55)))
            bg = tuple(
                int(BRAND[i] + (BRAND2[i] - BRAND[i]) * t) for i in range(3)
            )

            # 书本轮廓：边框带
            d_book = abs(rounded_rect_sdf(x, y, *BOOK))
            on_book = d_book <= half_stroke

            # 两条横线（圆头）
            on_line = any(
                seg_dist(x, y, *l) <= half_line for l in LINES
            )

            px[pxi, py] = (*(WHITE if (on_book or on_line) else bg), 255)

    return img.resize((size, size), Image.LANCZOS)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    out_ico = os.path.join(here, "app.ico")
    out_png = os.path.join(here, "app-icon.png")

    sizes = [16, 24, 32, 48, 64, 128, 256]
    imgs = [render(s) for s in sizes]

    imgs[-1].save(out_png)
    print("预览图:", out_png)

    imgs[-1].save(  # 以最大尺寸 256px 为主图，才能覆盖全部尺寸
        out_ico,
        format="ICO",
        sizes=[(s, s) for s in sizes],
    )
    print("图标:", out_ico, os.path.getsize(out_ico), "bytes")


if __name__ == "__main__":
    main()