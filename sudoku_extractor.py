import cv2
import numpy as np
import pytesseract
import sys

def extract_sudoku_grid(image_path):
    # Load image
    src = cv2.imread(image_path)
    if src is None:
        raise ValueError("Could not load image")

    # Preprocess: grayscale, blur, adaptive threshold
    gray = cv2.cvtColor(src, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (11, 11), 0)
    thresh = cv2.adaptiveThreshold(blurred, 255, cv2.ADAPTIVE_THRESH_MEAN_C, cv2.THRESH_BINARY_INV, 15, 5)

    # Find contours and select the largest one (Sudoku grid)
    contours, _ = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    max_area = 0
    largest_contour = None
    for cnt in contours:
        area = cv2.contourArea(cnt)
        if area > 50 and area > max_area:
            max_area = area
            largest_contour = cnt

    if largest_contour is None:
        raise ValueError("No Sudoku grid found")

    # Approximate to polygon (quadrilateral)
    peri = cv2.arcLength(largest_contour, True)
    approx = cv2.approxPolyDP(largest_contour, 0.01 * peri, True)
    pts = approx.reshape(4, 2).astype(np.float32)

    # Order points: top-left, top-right, bottom-right, bottom-left
    sums = np.sum(pts, axis=1)
    diffs = np.diff(pts, axis=1)
    ordered_pts = np.zeros((4, 2), dtype=np.float32)
    ordered_pts[0] = pts[np.argmin(sums)]  # TL: min sum
    ordered_pts[2] = pts[np.argmax(sums)]  # BR: max sum
    ordered_pts[1] = pts[np.argmin(diffs)]  # TR: min diff
    ordered_pts[3] = pts[np.argmax(diffs)]  # BL: max diff

    # Target square (450x450)
    side = 450
    dst_pts = np.array([[0, 0], [side - 1, 0], [side - 1, side - 1], [0, side - 1]], dtype=np.float32)

    # Warp perspective
    M = cv2.getPerspectiveTransform(ordered_pts, dst_pts)
    warped = cv2.warpPerspective(src, M, (side, side))

    # Preprocess warped image
    warped_gray = cv2.cvtColor(warped, cv2.COLOR_BGR2GRAY)
    warped_blurred = cv2.GaussianBlur(warped_gray, (11, 11), 0)
    warped_thresh = cv2.adaptiveThreshold(warped_blurred, 255, cv2.ADAPTIVE_THRESH_MEAN_C, cv2.THRESH_BINARY_INV, 15, 5)

    # Extract 9x9 grid
    grid = np.zeros((9, 9), dtype=int)
    cell_size = side // 9
    for i in range(9):
        for j in range(9):
            # Extract cell
            cell = warped_thresh[i * cell_size : (i + 1) * cell_size, j * cell_size : (j + 1) * cell_size]

            # Crop to avoid borders (e.g., 80% center)
            h, w = cell.shape
            margin = int(cell_size * 0.15)  # Adjust margin to focus on digit
            digit = cell[margin : h - margin, margin : w - margin]

            # Check if cell has content (threshold pixels)
            if cv2.countNonZero(digit) > 50:  # Adjust threshold as needed
                # Recognize digit with pytesseract
                config = r'--psm 10 --oem 3 -c tessedit_char_whitelist=123456789'
                text = pytesseract.image_to_string(digit, config=config).strip()
                if text.isdigit() and 1 <= int(text) <= 9:
                    grid[i, j] = int(text)

    return grid

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python sudoku_extractor.py <image_path>")
        sys.exit(1)
    
    try:
        grid = extract_sudoku_grid(sys.argv[1])
        for row in grid:
            print(" ".join(map(str, row)))
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)