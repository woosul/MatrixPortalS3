from PIL import Image, ImageDraw
import os

def create_minimal_test_gif():
    """매우 작은 테스트 GIF 생성 (8x8 픽셀)"""
    frames = []
    
    # 단순한 점 애니메이션
    for i in range(4):
        img = Image.new('RGB', (8, 8), (0, 0, 0))  # 검은 배경
        draw = ImageDraw.Draw(img)
        
        # 회전하는 점
        positions = [(2, 2), (5, 2), (5, 5), (2, 5)]
        x, y = positions[i]
        draw.rectangle([x, y, x+1, y+1], fill=(255, 0, 0))
        
        frames.append(img)
    
    # 매우 작은 GIF로 저장
    frames[0].save('./data/gifs/tiny_test.gif',
                   save_all=True,
                   append_images=frames[1:],
                   duration=250,
                   loop=0)
    
    print("Created tiny_test.gif (8x8 pixels)")

def create_simple_test_gif():
    """간단한 16x16 테스트 GIF"""
    frames = []
    
    for i in range(8):
        img = Image.new('RGB', (16, 16), (0, 0, 0))
        draw = ImageDraw.Draw(img)
        
        # 회전하는 선
        angle = i * 45
        center = 8
        if angle == 0:
            draw.line([(center, 2), (center, 14)], fill=(255, 255, 255), width=2)
        elif angle == 45:
            draw.line([(4, 4), (12, 12)], fill=(255, 255, 255), width=2)
        elif angle == 90:
            draw.line([(2, center), (14, center)], fill=(255, 255, 255), width=2)
        elif angle == 135:
            draw.line([(4, 12), (12, 4)], fill=(255, 255, 255), width=2)
        # 나머지 각도들...
        
        frames.append(img)
    
    frames[0].save('./data/gifs/simple_test.gif',
                   save_all=True,
                   append_images=frames[1:],
                   duration=200,
                   loop=0)
    
    print("Created simple_test.gif (16x16 pixels)")

if __name__ == "__main__":
    # data/gifs 디렉토리 생성
    os.makedirs('../data/gifs', exist_ok=True)
    
    create_minimal_test_gif()
    create_simple_test_gif()