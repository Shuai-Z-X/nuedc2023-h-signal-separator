import fitz
doc = fitz.open(r"C:\Users\Shuaizx\Desktop\23h\电赛报告.pdf")
page = doc[6]
# crop the figure-8 region (output stage schematic)
clip = fitz.Rect(40, 95, 560, 235)
mat = fitz.Matrix(400/72, 400/72)
pix = page.get_pixmap(matrix=mat, clip=clip)
pix.save(r"C:\Users\Shuaizx\Desktop\23h\tmp\fig8_output.png")
print(pix.width, pix.height)
