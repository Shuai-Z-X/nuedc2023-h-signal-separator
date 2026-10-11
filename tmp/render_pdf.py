import fitz
doc = fitz.open(r"C:\Users\Shuaizx\Desktop\23h\电赛报告.pdf")
page = doc[6]  # page 7
mat = fitz.Matrix(300/72, 300/72)
pix = page.get_pixmap(matrix=mat)
pix.save(r"C:\Users\Shuaizx\Desktop\23h\tmp\page7_300.png")
print(page.rect, pix.width, pix.height)
