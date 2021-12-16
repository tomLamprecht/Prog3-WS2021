import { Component, Input} from '@angular/core';
import { ItemModel } from 'src/lib/data-access/models/itemModel';


@Component({
  selector: 'column',
  templateUrl: './column.component.html',
  styleUrls: ['./column.component.scss'],
})
export class ColumnComponent{
  @Input()
  title = 'column'

  @Input()
  items: ItemModel[];
}
