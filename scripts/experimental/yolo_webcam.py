import cv2
from ultralytics import YOLO

def main():
    print("[INFO] Завантаження моделі YOLOv8 Nano...")
    # Автоматично завантажить файл yolov8n.pt (найлегша базова модель)
    model = YOLO("yolov8n.pt")
    
    print("[INFO] Ініціалізація веб-камери...")
    # 0 - це зазвичай вбудована веб-камера ноутбука
    cap = cv2.VideoCapture(0)
    
    if not cap.isOpened():
        print("[ERROR] Не вдалося відкрити веб-камеру.")
        return

    print("[INFO] Запуск відеопотоку. Натисніть 'q' для виходу.")
    
    while True:
        ret, frame = cap.read()
        if not ret:
            break
            
        # 1. Пропускаємо кадр через нейромережу (verbose=False прибирає зайвий спам у консоль)
        results = model.predict(source=frame, verbose=False)
        
        # Беремо результати для першого (і єдиного) кадру
        result = results[0]
        
        # 2. Отримуємо кадр з уже намальованими рамками для візуалізації
        annotated_frame = result.plot()
        
        # 3. Витягуємо сирі математичні дані для C++ Мозку
        for box in result.boxes:
            # Отримуємо центр, ширину та висоту рамки
            x_center, y_center, width, height = box.xywh[0]
            
            # Рівень впевненості (від 0 до 1) та ID класу
            confidence = float(box.conf[0])
            class_id = int(box.cls[0])
            class_name = model.names[class_id]
            
            # Фільтруємо: виводимо дані тільки якщо ШІ впевнений на >60%
            if confidence > 0.60:
                print(f"[ЦІЛЬ: {class_name.upper()}] "
                      f"X_центр: {x_center:.1f} | Y_центр: {y_center:.1f} | "
                      f"Ширина: {width:.1f} | Висота: {height:.1f} | "
                      f"Впевненість: {confidence*100:.1f}%")

        # 4. Виводимо картинку на екран
        cv2.imshow("YOLOv8 Tracker Test", annotated_frame)
        
        # Вихід з циклу при натисканні клавіші 'q'
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
            
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
