import fitz
doc = fitz.open(r"C:\Users\Shuaizx\Desktop\23h\电赛报告.pdf")
page = doc[7]
mat = fitz.Matrix(200/72, 200/72)
pix = page.get_pixmap(matrix=mat)
pix.save(r"C:\Users\Shuaizx\Desktop\23h\tmp\page8_full.png")
print(pix.width, pix.height)
