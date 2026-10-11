import fitz
doc = fitz.open(r"C:\Users\Shuaizx\Desktop\23h\电赛报告.pdf")
page = doc[7]
clip = fitz.Rect(60, 130, 545, 215)
mat = fitz.Matrix(500/72, 500/72)
pix = page.get_pixmap(matrix=mat, clip=clip)
pix.save(r"C:\Users\Shuaizx\Desktop\23h\tmp\fig8_zoom.png")
print(pix.width, pix.height)
