import numpy as np
import imageio.v2 as iio
import matplotlib.pyplot as plt
from sklearn.cluster import KMeans
import skimage.color as color
import skimage.transform as sk
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("image_path", type=str, help="Path to the input image")
parser.add_argument("--bins", type=int, default=4, help="Number of color bins for posterization")
args = parser.parse_args()


img = iio.imread(args.image_path)
# drop alpha channel if present
img = img[:, :, :3]
# resize for faster processing
img = sk.resize(img, (img.shape[0] // 6, img.shape[1] // 6), anti_aliasing=True)

# convert to HSV
img = color.rgb2hsv(img)

n_bins = args.bins
kmeans = KMeans(n_clusters=n_bins, random_state=0).fit(img.reshape(-1, 3))
labels = kmeans.predict(img.reshape(-1, 3))
posterized = color.hsv2rgb(kmeans.cluster_centers_[labels].reshape(img.shape))


fig, ax = plt.subplot_mosaic("AC;BC")
ax['A'].imshow(color.hsv2rgb(img))
ax['B'].imshow(posterized)

hsv_palette = kmeans.cluster_centers_
palette = color.hsv2rgb(hsv_palette)

# we want a high score when both saturation and lightness are high
# And a low score when either is low. Low lightness is worse than low saturation
palette_score = hsv_palette[:, 1] * (hsv_palette[:, 2] ** 2)
sorted_indices = np.argsort(-palette_score)
sorted_palette = palette[sorted_indices]
ax['C'].imshow(sorted_palette[:, np.newaxis, :])
plt.show()
