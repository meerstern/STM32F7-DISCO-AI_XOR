# import
from tensorflow import keras
import numpy as np
# Data
x_data = [
    [0,0],
    [0,1],
    [1,0],
    [1,1]
]
y_data = [
    [0],
    [1],
    [1],
    [0]
]
x_data = np.array(x_data)
y_data = np.array(y_data)
print(x_data.shape)
# (4,2,2,2)
print(y_data.shape)
# build the model
model = keras.Sequential()
model.add(keras.layers.Dense(8, activation="sigmoid", input_shape=(2,)))
model.add(keras.layers.Dense(1, activation="sigmoid"))
optimizer = keras.optimizers.SGD(lr=0.1)
model.compile(optimizer=optimizer, loss="binary_crossentropy", metrics=['accuracy'])
model.summary()
model.fit(x_data, y_data, batch_size=4, epochs=5000)
predict = model.predict(x_data)
print(np.round(predict))
model.save('XOR.h5')