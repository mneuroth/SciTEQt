// Simple Plot Demo for JavaScript in SciteQt
// Run this script by pressing Ctrl+J

function SimpleGraph(xStart,xStop,yStart,yStop,width=40,height=13) {
    this.x0 = xStart
    this.x1 = xStop
    this.y0 = yStart
    this.y1 = yStop
    this.width = width
    this.height = height
    this.dxScreenBydxLogical = (width-1)/(xStop-xStart)
    this.dyScreenBydyLogical = (height-1)/(yStop-yStart)
    this.screen = new Array(height)
    
    // set methods
    this.plot = plot
    this.pixel = pixel
    this.line = line
    this.checkScreenPoint = checkScreenPoint
    this.checkLogicalPoint = checkLogicalPoint
    this.logicalToScreenX = logicalToScreenX
    this.logicalToScreenY = logicalToScreenY
    this.showScreen = showScreen
    this._screenLine = _screenLine
    this._setScreenPoint = _setScreenPoint
    
    // init data
    for (var y = 0; y < this.screen.length; y++) {
        this.screen[y] = new Array(width)
        for (var x = 0; x <this.screen[y].length; x++) {
            this.screen[y][x] = " "
        }
    }
}

function plot(fcn) {
    var dx = (this.x1 - this.x0)/this.width
    
    for( var x = this.x0; x < this.x1; x += dx) {
        var y = fcn(x)
        this.pixel(x,y)
    }
}

function line(x1,y1,x2,y2,ch="*") {
    var xs1 = this.logicalToScreenX(x1)
    var xs2 = this.logicalToScreenX(x2)
    var ys1 = this.logicalToScreenY(y1)
    var ys2 = this.logicalToScreenY(y2)
    this._screenLine(xs1,ys1,xs2,ys2,ch)
}

function pixel(x,y,ch="*") {
    if( this.checkLogicalPoint(x,y) ) {
        var _x = this.logicalToScreenX(x)
        var _y = this.logicalToScreenY(y)
        this._setScreenPoint(_x,_y,ch)
    }
}

function checkScreenPoint(x,y) {
    return x >= 0 && x < this.width && y >= 0 && y < this.height 
}

function checkLogicalPoint(x,y) {
    return x >= this.x0 && x <= this.x1 && y >= this.y0 && y <= this.y1 
}

function logicalToScreenX(x) {
    return Math.round((x-this.x0)*this.dxScreenBydxLogical)
}

function logicalToScreenY(y) {
    return Math.round((y-this.y0)*this.dyScreenBydyLogical)
}

function showScreen() {
    //println("showScreen obj=",this,"width=",this.width,"height=",this.height);
    for( var y = this.screen.length-1; y >= 0; y-- ) {
        print(">")  // switch to monospace font in output area
        for( var x = 0; x < this.screen[y].length; x++ ) { 
            print(this.screen[y][x])
        }
        println()
    }    
}

function _setScreenPoint(x,y,ch="*") {
    if( this.checkScreenPoint(x,y) ) {
        for( var i = 0; i < ch.length; i++ ) {
            this.screen[y][x+i] = ch[i]
        }
    }
}

// see: https://jstutorial.medium.com/how-to-code-your-first-algorithm-draw-a-line-ca121f9a1395
function _screenLine(x1,y1,x2,y2,ch="*") {
    // Iterators, counters required by algorithm
    let x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;    
    
    // Calculate line deltas
    dx = x2 - x1;
    dy = y2 - y1;    
    
    // Create a positive copy of deltas (makes iterating easier)
    dx1 = Math.abs(dx);
    dy1 = Math.abs(dy);    
    
    // Calculate error intervals for both axis
    px = 2 * dy1 - dx1;
    py = 2 * dx1 - dy1;   
    
    // The line is X-axis dominant
    if (dy1 <= dx1) {        
        // Line is drawn left to right
        if (dx >= 0) {
            x = x1; y = y1; xe = x2;
        } else { // Line is drawn right to left (swap ends)
            x = x2; y = y2; xe = x1;
        }     
        
        this._setScreenPoint(x, y, ch); // Draw first pixel        
        
        // Rasterize the line
        for (i = 0; x < xe; i++) {
            x = x + 1;            
            
            // Deal with octants...
            if (px < 0) {
                px = px + 2 * dy1;
            } else {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
                    y = y + 1;
                } else {
                    y = y - 1;
                }
                px = px + 2 * (dy1 - dx1);
            }            
            
            // Draw pixel from line span at
            // currently rasterized position
            this._setScreenPoint(x, y, ch);
        }    
        
    } else { // The line is Y-axis dominant        
            
        // Line is drawn bottom to top
        if (dy >= 0) {
            x = x1; y = y1; ye = y2;
        } else { // Line is drawn top to bottom
            x = x2; y = y2; ye = y1;
        }    
        
        this._setScreenPoint(x, y, ch); // Draw first pixel        
        
        // Rasterize the line
        for (i = 0; y < ye; i++) {
            y = y + 1;            
            
            // Deal with octants...
            if (py <= 0) {
                py = py + 2 * dx1;
            } else {
                if ((dx < 0 && dy<0) || (dx > 0 && dy > 0)) {
                    x = x + 1;
                } else {
                    x = x - 1;
                }
                py = py + 2 * (dx1 - dy1);
            }        
            
            // Draw pixel from line span at
            // currently rasterized position
            this._setScreenPoint(x, y, ch);
        }
    }
}

//#########################################################################

function main() {
    var xmin = -Math.PI
    var xmax = Math.PI
    var ymin = -1
    var ymax = 1
    var graph = new SimpleGraph(xmin,xmax,ymin,ymax)     
    graph.line(xmin,0,xmax,0,"-")
    graph.line(0,ymin,0,ymax,"|")
    graph.pixel(-1,0,"-1")
    graph.pixel(1,0,"1")
    graph.pixel(0,-1,"-1")
    graph.pixel(0,1,"1")
    graph.plot(function (x) { return Math.sin(x) })
    graph.showScreen()
}

main()
