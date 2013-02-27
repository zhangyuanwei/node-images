node-images
===========

Cross-platform image decoder(png/jpeg/gif) and encoder(png) for Nodejs  
Node.js轻量级跨平台图像编解码库

## Installation 安装
	$ npm install images

## Usage 使用

``` javascript
var images = require("images");

images("input.jpg")                     //Load image from file 
                                        //加载图像文件
    .size(400)                          //Geometric scaling the image to 400 pixels width
                                        //等比缩放图像到400像素宽
    .draw(images("logo.png"), 10, 10)   //Drawn logo at coordinates (10,10)
                                        //在(10,10)处绘制Logo
    .save("output.png")                //Save the image to a file
                                        //保存图片到文件
```

## API 接口

node-images provide jQuery-like Chaining API,You can start the chain like this:  
node-images 提供了类似jQuery的链式调用API,您可以这样开始:

```javascript
/* Load and decode image from file */
/* 从指定文件加载并解码图像 */
images(file)

/* Create a new transparent image */
/* 创建一个指定宽高的透明图像 */
images(width, height)

/* Load and decode image from a buffer */
/* 从Buffer数据中解码图像 */
images(buffer[, start[, end]])

/* Copy from another image */
/* 从另一个图像中复制区域来创建图像 */
images(image[, x, y, width, height])
```

### images(file)

Load and decode image from file  
从指定文件加载并解码图像


### images(width, height)

Create a new transparent image  
创建一个指定宽高的透明图像


### images(buffer[, start[, end]])

Load and decode image from a buffer  
从Buffer数据中解码图像


### images(image[, x, y, width, height])

Copy from another image  
从另一个图像中复制区域来创建图像


### .fill(red, green, blue[, alpha])

Fill image with color  
以指定颜色填充图像


### .draw(image, x, y)

Draw *image* on the current image position(*x*, *y*)  
在当前图像(*x*, *y*)上绘制 *image* 图像


### .encode(type)

Encode image to buffer  
以指定格式编码当前图像到Buffer  
Return buffer  
返回填充好的Buffer  
**Note:The operation will cut off the chain**  
**注意:该操作将会切断调用链**  


### .save(file[, type])

Encoding and save the current image to a *file*, if the *type* is not specified, *type* well be automatically determined according to the *file*  
编码并保存当前图像到*file*,如果type未指定,则根据*file*自动判断文件类型


### .size([width[, height]])

Get size of the image or set the size of the image,if the height is not specified, then scaling based on the current width and height  
获取或者设置图像宽高，如果height未指定，则根据当前宽高等比缩放


### .width([width])

Get width for the image or set width of the image  
获取或设置图像宽度


### .height([height])

Get height for the image or set height of the image  
获取或设置图像高度


### images.setLimit(width, height)

Set the limit size of each image  
设置图片大小限制
