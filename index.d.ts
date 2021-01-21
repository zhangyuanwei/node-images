/// <reference types="node" />
export = images;

declare namespace images {
    interface EncoderConfig {
        [key: string]: number | string | undefined;

        quality?: number;
    }

    type FILE_TYPE = "png" | "jpg" | "jpeg" | "gif" | "bmp" | "raw";

    const enum TYPE {
        TYPE_PNG = 1,
        TYPE_JPEG,
        TYPE_GIF,
        TYPE_BMP,
        TYPE_RAW,
        TYPE_WEBP,
    }

    class Image {
        loadFromBuffer(buffer: Buffer, start?: number, end?: number): this;

        copyFromImage(img: Image, x: number, y: number, width: number, height: number): this;
        copyFromImage(img: Image, x: number, y: number): this;
        copyFromImage(img: Image): this;

        fill(red: number, green: number, blue: number, alpha?: number): this;

        fillColor(red: number, green: number, blue: number, alpha?: number): this;;

        draw(img: Image, x: number, y: number): this;

        drawImage(img: Image, x: number, y: number): this;

        encode(type: FILE_TYPE | TYPE, config?: EncoderConfig): Buffer;

        toBuffer(type: FILE_TYPE | TYPE, config?: EncoderConfig): Buffer;

        save(file: string, type: FILE_TYPE | TYPE, config: EncoderConfig): this;
        save(file: string, config: EncoderConfig): this;
        save(file: string, type: FILE_TYPE | TYPE): this;
        save(file: string): this;

        saveAsync(file: string, type: FILE_TYPE | TYPE, config: EncoderConfig, callback: (err: NodeJS.ErrnoException | null) => void): this;
        saveAsync(file: string, type: FILE_TYPE | TYPE, callback: (err: NodeJS.ErrnoException | null) => void): this;
        saveAsync(file: string, config: EncoderConfig, callback: (err: NodeJS.ErrnoException | null) => void): this;
        saveAsync(file: string, callback: (err: NodeJS.ErrnoException | null) => void): this;

        resize(width: number, height?: number, filter?: string): this;

        rotate(deg: number): this;

        size(): { width: number, height: number };
        size(width: number, height?: number): this;

        width(): number;
        width(width: number): this;

        height(): number;
        height(height: number): this;
    }

    function loadFromFileAsync(file: string): Promise<Image>;
    function loadFromFileNew(filePath: string): Image;
    function loadFromFile(file: string): Image;

    function createImage(width?: number, height?: number): Image;

    function loadFromBuffer(buffer: Buffer, start?: number, end?: number): Image;

    function copyFromImage(image: Image, x?: number, y?: number, width?: number, height?: number): Image;

    function setLimit(maxWidth: number, maxHeight: number): void;

    function setGCThreshold(value: number): void;

    function getUsedMemory(): number;

    function gc(): void;

}

declare function images(buffer: Buffer, start?: number, end?: number): images.Image;
declare function images(image: images.Image, x: number, y: number, width: number, height: number): images.Image;
declare function images(image: images.Image, x: number, y: number): images.Image;
declare function images(image: images.Image): images.Image;
declare function images(file: string): images.Image;
declare function images(width?: number, height?: number): images.Image;