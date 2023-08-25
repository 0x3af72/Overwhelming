# code taken from https://stackoverflow.com/questions/2672144/dump-characters-glyphs-from-truetype-font-ttf-into-bitmaps

from PIL import Image, ImageFont, ImageDraw
import string

# use a truetype font
font = ImageFont.truetype("font.ttf", 60)
im = Image.new("RGBA", (64, 64))
draw = ImageDraw.Draw(im)

for char in string.ascii_letters + string.digits:
  w, h = draw.textsize(char, font=font)
  im = Image.new("RGBA", (w, h))
  draw = ImageDraw.Draw(im)
  draw.text((-2, 0), char, font=font, fill="#FFFFFF")
  im.save(char + ".png")